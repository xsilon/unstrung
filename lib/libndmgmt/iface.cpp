/*
 * Copyright (C) 2009 Michael Richardson <mcr@sandelman.ca>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include <errno.h>
#include <pathnames.h>		/* for PATH_PROC_NET_IF_INET6 */
#include <poll.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <linux/if.h>           /* for IFNAMSIZ */
#include "rpl.h"

#ifndef IPV6_ADDR_LINKLOCAL
#define IPV6_ADDR_LINKLOCAL   0x0020U
#endif
}

#include "iface.h"


network_interface::network_interface()
{
    nd_socket = -1;
    alive = false;
}

network_interface::network_interface(int fd)
{
    nd_socket = fd;
    alive = true;
}

network_interface::network_interface(const char *if_name)
{
    alive = false;
    nd_socket = -1;

    this->set_if_name(if_name);
}

void network_interface::set_if_name(const char *if_name)
{
    this->if_name[0]=0;
    strncat(this->if_name, if_name, sizeof(this->if_name));
}

void network_interface::set_rpl_dagid(const char *dagstr)
{
    if(dagstr[0]=='0' && dagstr[1]=='x') {
        const char *digits;
        int i;
        digits = dagstr+2;
        for(i=0; i<16 && *digits!='\0'; i++) {
            unsigned int value;
            if(sscanf(digits, "%2x",&value)==0) break;
            this->rpl_dagid[i]=value;
            
            /* advance two characters, carefully */
            digits++;
            if(digits[0]) digits++;
        }
    } else {
        int len = strlen(dagstr);
        if(len > 16) len=16;
        
        memset(this->rpl_dagid, 0, 16);
        memcpy(this->rpl_dagid, dagstr, len);
    }
}

bool network_interface::setup()
{
    if(alive && nd_socket != -1) return true;

    if(VERBOSE(this)) {
        fprintf(this->verbose_file, "Starting setup for %s\n", this->if_name);
    }
    
    nd_socket = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    struct icmp6_filter filter;
    int err, val;

    if (nd_socket < 0)
    {
        fprintf(this->verbose_file, "can't create socket(AF_INET6): %s", strerror(errno));
	return false;
    }

    val = 1;
    err = setsockopt(nd_socket, IPPROTO_IPV6, IPV6_RECVPKTINFO, &val, sizeof(val));
    if (err < 0)
    {
        fprintf(this->verbose_file, "setsockopt(IPV6_RECVPKTINFO): %s", strerror(errno));
        return false;
    }

    val = 2;
    err = setsockopt(nd_socket, IPPROTO_RAW, IPV6_CHECKSUM, &val, sizeof(val));
    if (err < 0)
    {
        fprintf(this->verbose_file, "setsockopt(IPV6_CHECKSUM): %s", strerror(errno));
        return false;
    }

    val = 255;
    err = setsockopt(nd_socket, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &val, sizeof(val));
    if (err < 0)
    {
        fprintf(this->verbose_file, "setsockopt(IPV6_UNICAST_HOPS): %s", strerror(errno));
        return false;
    }
    
    val = 255;
    err = setsockopt(nd_socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &val, sizeof(val));
    if (err < 0)
    {
        fprintf(this->verbose_file, "setsockopt(IPV6_MULTICAST_HOPS): %s", strerror(errno));
        return false;
    }

#ifdef IPV6_RECVHOPLIMIT
    val = 1;
    err = setsockopt(nd_socket, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &val, sizeof(val));
    if (err < 0)
    {
        fprintf(this->verbose_file, "setsockopt(IPV6_RECVHOPLIMIT): %s", strerror(errno));
        return false;
    }
#endif

#if 0
    /*
     * setup ICMP filter
     */
    
    ICMP6_FILTER_SETBLOCKALL(&filter);
    ICMP6_FILTER_SETPASS(ND_RPL_MESSAGE,    &filter);
    ICMP6_FILTER_SETPASS(ND_ROUTER_SOLICIT, &filter);
    ICMP6_FILTER_SETPASS(ND_ROUTER_ADVERT, &filter);
    
    err = setsockopt(nd_socket, IPPROTO_ICMPV6, ICMP6_FILTER, &filter,
                     sizeof(filter));
    if (err < 0)
    {
        fprintf(this->verbose_file, "setsockopt(ICMPV6_FILTER): %s", strerror(errno));
        return false;
    }
#endif

    setup_allrouters_membership();
    struct ipv6_mreq mreq;                  
	
    alive = true;
    add_to_list();
    
    return true;
}

void network_interface::setup_allrouters_membership(void)
{
	struct ipv6_mreq mreq;                  
	
	memset(&mreq, 0, sizeof(mreq));                  
	mreq.ipv6mr_interface = this->get_if_index();
	
	/* ipv6-allrouters: ff02::2 */
	mreq.ipv6mr_multiaddr.s6_addr32[0] = htonl(0xFF020000);
	mreq.ipv6mr_multiaddr.s6_addr32[3] = htonl(0x2);     

        if (setsockopt(nd_socket, SOL_IPV6, IPV6_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
        {
            /* linux-2.6.12-bk4 returns error with HUP signal but keep listening */
            if (errno != EADDRINUSE)
            {
                printf("can't join ipv6-allrouters on %s", this->if_name);
                alive = false;
                return;
            }
        }

	return;
}

void network_interface::check_allrouters_membership(void)
{
	#define ALL_ROUTERS_MCAST "ff020000000000000000000000000002"
	
	FILE *fp;
	unsigned int if_idx, allrouters_ok=0;
	char addr[32+1];
	int ret=0;

	if ((fp = fopen(PATH_PROC_NET_IGMP6, "r")) == NULL)
	{
		printf("can't open %s: %s", PATH_PROC_NET_IGMP6,
			strerror(errno));
		return;	
	}
	
	while ( (ret=fscanf(fp, "%u %*s %32[0-9A-Fa-f] %*x %*x %*x\n", &if_idx, addr)) != EOF) {
            if (ret == 2) {
                if (this->if_index == if_idx) {
                    if (strncmp(addr, ALL_ROUTERS_MCAST, sizeof(addr)) == 0)
                        allrouters_ok = 1;
                }
            }
	}

	fclose(fp);

	if (!allrouters_ok) {
            printf("resetting ipv6-allrouters membership on %s\n", this->if_name);
            setup_allrouters_membership();
	}	

	return;
}		


class network_interface *network_interface::all_if = NULL;
void network_interface::add_to_list(void)
{
    this->next = network_interface::all_if;
    network_interface::all_if = this;
}

/* this searches through /proc to get right ifindex */
int network_interface::get_if_index(void)
{
    if(this->if_index > 0) {
        return this->if_index;
    }

    /*
     * this function extracts the link local address and interface index
     * from PATH_PROC_NET_IF_INET6. 
     */
    FILE *fp;
    char str_addr[40];
    unsigned int plen, scope, dad_status, if_idx;
    char devname[IFNAMSIZ];

    if ((fp = fopen(PATH_PROC_NET_IF_INET6, "r")) == NULL)
    {
        fprintf(this->verbose_file, "can't open %s: %s", PATH_PROC_NET_IF_INET6,
                strerror(errno));
        return -1;	
    }
	
    while (fscanf(fp, "%32s %x %02x %02x %02x %15s\n",
                  str_addr, &if_idx, &plen, &scope, &dad_status,
                  devname) != EOF)
    {
        if (scope == IPV6_ADDR_LINKLOCAL &&
            strcmp(devname, this->if_name) == 0)
        {
            struct in6_addr addr;
            unsigned int ap;
            int i;
			
            for (i=0; i<16; i++)
            {
                sscanf(str_addr + i * 2, "%02x", &ap);
                addr.s6_addr[i] = (unsigned char)ap;
            }
            memcpy(&this->if_addr, &addr, sizeof(this->if_addr));
            
            this->if_index = if_idx;
            fclose(fp);
            return this->if_index;
        }
    }
    
    fprintf(this->verbose_file, "no linklocal address configured for %s",
            this->if_name);
    fclose(fp);
    return -1;
}

void network_interface::receive_packet(struct in6_addr ip6_src,
				       struct in6_addr ip6_dst,
				       const u_char *bytes, const int len)
{
    const struct nd_opt_hdr *nd_options;
    char src_addrbuf[INET6_ADDRSTRLEN];
    char dst_addrbuf[INET6_ADDRSTRLEN];

    /* should collect this all into a "class packet", or "class transaction" */
    if(this->verbose_flag) {
	inet_ntop(AF_INET6, &ip6_src, src_addrbuf, INET6_ADDRSTRLEN);
	inet_ntop(AF_INET6, &ip6_dst, dst_addrbuf, INET6_ADDRSTRLEN);
    }

    struct icmp6_hdr *icmp6 = (struct icmp6_hdr *)bytes;
    
    /* mark end of data received */
    const u_char *bytes_end = bytes + len;

    /* XXX should maybe check the checksum? */

    switch(icmp6->icmp6_type) {
    case ND_RPL_MESSAGE:
        switch(icmp6->icmp6_code) {
        case ND_RPL_DAG_IO:
            this->receive_dio(icmp6->icmp6_data8, bytes_end - icmp6->icmp6_data8);
            break;

        default:
            if(VERBOSE(this)) {
                fprintf(this->verbose_file, "Got unknown RPL code: %u\n", icmp6->icmp6_code);
            }
            break;
        }
        return;
            
    case ND_ROUTER_SOLICIT:
    {
	struct nd_router_solicit *nrs = (struct nd_router_solicit *)bytes;
	nd_options = (const struct nd_opt_hdr *)&nrs[1];
	if(VERBOSE(this)) {
	    fprintf(this->verbose_file, "Got router solicitation from %s with %u bytes of options\n",
		    src_addrbuf,
		    bytes_end - (u_char *)nd_options);
	}
    }
    break;

    case ND_ROUTER_ADVERT:
    {
	struct nd_router_advert *nra = (struct nd_router_advert *)bytes;
	nd_options = (const struct nd_opt_hdr *)&nra[1];
	if(VERBOSE(this)) {
	    fprintf(this->verbose_file, "Got router advertisement from %s (hoplimit: %u, flags=%s%s%s, lifetime=%ums, reachable=%u, retransmit=%u)\n  with %u bytes of options\n",
		    src_addrbuf,
		    nra->nd_ra_curhoplimit,
		    nra->nd_ra_flags_reserved & ND_RA_FLAG_MANAGED ? "managed " : "",
		    nra->nd_ra_flags_reserved & ND_RA_FLAG_OTHER ? "other " : "",
		    nra->nd_ra_flags_reserved & ND_RA_FLAG_HOME_AGENT ? "home-agent " : "",
		    ntohs(nra->nd_ra_router_lifetime),
		    ntohl(nra->nd_ra_reachable),
		    ntohl(nra->nd_ra_retransmit),
		    bytes_end - (u_char *)nd_options);
	}
    }
    break;

    case ND_NEIGHBOR_SOLICIT:
    {
	struct nd_neighbor_solicit *nns = (struct nd_neighbor_solicit *)bytes;
	nd_options = (const struct nd_opt_hdr *)&nns[1];
	if(VERBOSE(this)) {
	    char target_addrbuf[INET6_ADDRSTRLEN];
	    inet_ntop(AF_INET6, &nns->nd_ns_target, target_addrbuf, INET6_ADDRSTRLEN);
	    fprintf(this->verbose_file, "Got neighbour solicitation from %s, looking for %s, has %u bytes of options\n",
		    src_addrbuf,
		    target_addrbuf,
		    bytes_end - (u_char *)nd_options);
	}
    }
    break;
	
    case ND_NEIGHBOR_ADVERT:
    {
	struct nd_neighbor_advert *nna = (struct nd_neighbor_advert *)bytes;
	nd_options = (const struct nd_opt_hdr *)&nna[1];
	if(VERBOSE(this)) {
	    char target_addrbuf[INET6_ADDRSTRLEN];
	    inet_ntop(AF_INET6, &nna->nd_na_target, target_addrbuf, INET6_ADDRSTRLEN);
	    fprintf(this->verbose_file, "Got neighbor advertisement from %s (flags=%s%s%s), advertising: %s, has %u bytes of options\n",
		    src_addrbuf,
		    nna->nd_na_flags_reserved & ND_NA_FLAG_ROUTER ? "router " : "",
		    nna->nd_na_flags_reserved & ND_NA_FLAG_SOLICITED ? "solicited " : "",
		    nna->nd_na_flags_reserved & ND_NA_FLAG_OVERRIDE  ? "override " : "",
		    
		    target_addrbuf,
		    bytes_end - (u_char *)nd_options);
	}
    }
    break;
    }

    /* now decode the option packets */
    while((const u_char *)nd_options < bytes_end && nd_options->nd_opt_type!=0) {
	/*
	 * option lengths are in units of 64-bits, the minimum size for
	 * an IPv6 extension
	 */
	unsigned int optlen = nd_options->nd_opt_len << 3;
	if(this->packet_too_short("option",
				  optlen,
				  sizeof(struct nd_opt_hdr))) return;

	if(this->packet_too_short("option",
				  (bytes_end - bytes),
				  optlen)) return;

	if(VERBOSE(this)) {
	    fprintf(this->verbose_file, "    option %u[%u]",
		    nd_options->nd_opt_type, optlen);
	}
	switch(nd_options->nd_opt_type) {
	case ND_OPT_SOURCE_LINKADDR:
	    if(VERBOSE(this)) fprintf(this->verbose_file, " source-linkaddr \n");
	    break;

	case ND_OPT_TARGET_LINKADDR:
	    if(VERBOSE(this)) fprintf(this->verbose_file, " target-linkaddr \n");
	    break;

	case ND_OPT_PREFIX_INFORMATION:
	{
	    struct nd_opt_prefix_info *nopi = (struct nd_opt_prefix_info *)nd_options;
	    if(this->packet_too_short("option",
				      optlen,
				      sizeof(struct nd_opt_prefix_info))) return;
	    
	    if(VERBOSE(this)) {
		char prefix_addrbuf[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &nopi->nd_opt_pi_prefix, prefix_addrbuf, INET6_ADDRSTRLEN);
		fprintf(this->verbose_file, " prefix %s/%u (valid=%us, preferred=%us) flags=%s%s%s\n",
			prefix_addrbuf, nopi->nd_opt_pi_prefix_len,
			nopi->nd_opt_pi_valid_time,
			nopi->nd_opt_pi_preferred_time,
			nopi->nd_opt_pi_flags_reserved & ND_OPT_PI_FLAG_ONLINK ? "onlink " : "",
			nopi->nd_opt_pi_flags_reserved & ND_OPT_PI_FLAG_AUTO   ? "auto " : "",
			nopi->nd_opt_pi_flags_reserved & ND_OPT_PI_FLAG_RADDR  ? "raddr " : "");
	    }
	}
	break;

	case ND_OPT_REDIRECTED_HEADER:
	    if(VERBOSE(this)) fprintf(this->verbose_file, " redirected \n");
	    break;

	case ND_OPT_MTU:
	    if(VERBOSE(this)) fprintf(this->verbose_file, " mtu \n");
	    break;

	case ND_OPT_RTR_ADV_INTERVAL:
	    if(VERBOSE(this)) fprintf(this->verbose_file, " rtr-interval \n");
	    break;

	case ND_OPT_HOME_AGENT_INFO:
	    if(VERBOSE(this)) fprintf(this->verbose_file, " home-agent-info \n");
	    break;

	case ND_OPT_RPL_PRIVATE_DAO:
	    /* SHOULD validate that this arrived in an NA */
	    if(VERBOSE(this)) fprintf(this->verbose_file, " rpl_dao \n");
            {
                const u_char *nd_dao = (u_char *)nd_options;
                this->receive_dao(nd_dao+2, optlen-2);
            }
	    break;

	case ND_OPT_RPL_PRIVATE_DIO:
	    /* SHOULD validate that this arrived in an RA */
	    if(VERBOSE(this)) fprintf(this->verbose_file, " rpl_dio \n");
            {
                const u_char *nd_dio = (u_char *)nd_options;
                this->receive_dio(nd_dio+2, optlen-2);
            }
	    break;

	default:
	    /* nothing */
	    break;
	}
	
	const u_char *nd_bytes = (u_char *)nd_options;
	nd_bytes += optlen;
	nd_options = (const struct nd_opt_hdr *)nd_bytes;
    }
    
    
}

int network_interface::packet_too_short(const char *thing,
				       const int avail_len,
				       const int needed_len)
{
    if(avail_len < needed_len) {
	fprintf(this->verbose_file, "%s has invalid len: %u(<%u)\n",
		thing, avail_len, needed_len);
	
	this->error_cnt++;
	/* discard packet */
	return 1;
    }
    return 0;
}

void network_interface::receive(void)
{
    unsigned char b[2048];
    struct sockaddr_in6 src_sock;
    struct in6_addr src, dst;
    int len, n;
    struct msghdr mhdr;
    struct iovec iov;
    int hoplimit = 0;
    struct in6_pktinfo *pkt_info;

    if( ! control_msg_hdr )
    {
        control_msg_hdrlen = CMSG_SPACE(sizeof(struct in6_pktinfo)) +
            CMSG_SPACE(sizeof(int));
        if ((control_msg_hdr = (unsigned char *)malloc(control_msg_hdrlen)) == NULL) {
            fprintf(this->verbose_file, "recv_rs_ra: malloc: %s", strerror(errno));
            return;
        }
    }

    iov.iov_len = sizeof(b);
    iov.iov_base = (caddr_t) b;

    memset(&mhdr, 0, sizeof(mhdr));
    mhdr.msg_name = (caddr_t)&src_sock;
    mhdr.msg_namelen = sizeof(src_sock);
    mhdr.msg_iov = &iov;
    mhdr.msg_iovlen = 1;
    mhdr.msg_control = (void *)control_msg_hdr;
    mhdr.msg_controllen = control_msg_hdrlen;

    if((len = recvmsg(this->nd_socket, &mhdr, 0)) >= 0) {
        struct cmsghdr *cmsg;

        for (cmsg = CMSG_FIRSTHDR(&mhdr); cmsg != NULL; cmsg = CMSG_NXTHDR(&mhdr, cmsg))
	{
            if (cmsg->cmsg_level != IPPROTO_IPV6)
          	continue;
            
            switch(cmsg->cmsg_type)
            {
#ifdef IPV6_HOPLIMIT
            case IPV6_HOPLIMIT:
                if ((cmsg->cmsg_len == CMSG_LEN(sizeof(int))) && 
                    (*(int *)CMSG_DATA(cmsg) >= 0) && 
                    (*(int *)CMSG_DATA(cmsg) < 256))
                {
                    hoplimit = *(int *)CMSG_DATA(cmsg);
                }
                else
                {
                    fprintf(this->verbose_file, "received a bogus IPV6_HOPLIMIT from the kernel! len=%d, data=%d",
                            cmsg->cmsg_len, *(int *)CMSG_DATA(cmsg));
                    return;	
                }  
                break;
#endif /* IPV6_HOPLIMIT */

            case IPV6_PKTINFO:
                if ((cmsg->cmsg_len == CMSG_LEN(sizeof(struct in6_pktinfo))) &&
                    ((struct in6_pktinfo *)CMSG_DATA(cmsg))->ipi6_ifindex)
                {
                    pkt_info = (struct in6_pktinfo *)CMSG_DATA(cmsg);
                }
                else
                {
                    fprintf(this->verbose_file, "received a bogus IPV6_PKTINFO from the kernel! len=%d, index=%d", 
                            cmsg->cmsg_len, ((struct in6_pktinfo *)CMSG_DATA(cmsg))->ipi6_ifindex);
                    return;
                } 
                break;
            }
	}

        dst = pkt_info->ipi6_addr;
        src = src_sock.sin6_addr;
        
        if(VERBOSE(this)) {
            char src_addrbuf[INET6_ADDRSTRLEN];
            char dst_addrbuf[INET6_ADDRSTRLEN];

            inet_ntop(AF_INET6, &src, src_addrbuf, INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &dst, dst_addrbuf, INET6_ADDRSTRLEN);

            fprintf(this->verbose_file, "received packet from %s -> %s[%u]\n",
                    src_addrbuf, dst_addrbuf, pkt_info->ipi6_ifindex);
        }

        this->receive_packet(src, dst, b, len);
    }
    
}

int network_interface::send_packet(const u_char *bytes, const int len)
{
    setup();

    check_allrouters_membership();    
}

void network_interface::send_raw_dio(unsigned char *icmp_body, unsigned int icmp_len)
{
    uint8_t all_hosts_addr[] = {0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    struct sockaddr_in6 addr;
    struct in6_addr *dest = NULL;
    struct in6_pktinfo *pkt_info;
    struct msghdr mhdr;
    struct cmsghdr *cmsg;
    struct iovec iov;
    char __attribute__((aligned(8))) chdr[CMSG_SPACE(sizeof(struct in6_pktinfo))];
    
    int err;
    
    setup();
    check_allrouters_membership();    

    printf("sending RA on %u\n", nd_socket);
    
    if (dest == NULL)
    {
        dest = (struct in6_addr *)all_hosts_addr;
        update_multicast_time();
    }
    
    memset((void *)&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(IPPROTO_ICMPV6);
    memcpy(&addr.sin6_addr, dest, sizeof(struct in6_addr));

    iov.iov_len  = icmp_len;
    iov.iov_base = (caddr_t) icmp_body;
    
    memset(chdr, 0, sizeof(chdr));
    cmsg = (struct cmsghdr *) chdr;
    
    cmsg->cmsg_len   = CMSG_LEN(sizeof(struct in6_pktinfo));
    cmsg->cmsg_level = IPPROTO_IPV6;
    cmsg->cmsg_type  = IPV6_PKTINFO;
    
    pkt_info = (struct in6_pktinfo *)CMSG_DATA(cmsg);
    pkt_info->ipi6_ifindex = this->get_if_index();
    memcpy(&pkt_info->ipi6_addr, &this->if_addr, sizeof(struct in6_addr));
    
#ifdef HAVE_SIN6_SCOPE_ID
    if (IN6_IS_ADDR_LINKLOCAL(&addr.sin6_addr) ||
        IN6_IS_ADDR_MC_LINKLOCAL(&addr.sin6_addr))
        addr.sin6_scope_id = iface->if_index;
#endif
    
    memset(&mhdr, 0, sizeof(mhdr));
    mhdr.msg_name = (caddr_t)&addr;
    mhdr.msg_namelen = sizeof(struct sockaddr_in6);
    mhdr.msg_iov = &iov;
    mhdr.msg_iovlen = 1;
    mhdr.msg_control = (void *) cmsg;
    mhdr.msg_controllen = sizeof(chdr);
    
    err = sendmsg(nd_socket, &mhdr, 0);
    
    if (err < 0) {
        printf("send_raw_dio/sendmsg: %s\n", strerror(errno));
    }
}

int network_interface::build_dio(unsigned char *buff, unsigned int buff_len)
{
    uint8_t all_hosts_addr[] = {0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    struct sockaddr_in6 addr;
    struct in6_addr *dest = NULL;
    struct icmp6_hdr  *icmp6;
    struct nd_rpl_dio *dio;
    int len = 0;
    
    memset(buff, 0, buff_len);
    
    icmp6 = (struct icmp6_hdr *)buff;
    icmp6->icmp6_type = ND_RPL_MESSAGE;
    icmp6->icmp6_code = ND_RPL_DAG_IO;
    icmp6->icmp6_cksum = 0;
    
    dio = (struct nd_rpl_dio *)icmp6->icmp6_data8;
    
    dio->rpl_flags = 0;
    if(this->rpl_grounded) {
        dio->rpl_flags |= ND_RPL_DIO_GROUNDED;
    }
    dio->rpl_seq        = this->rpl_sequence;
    dio->rpl_instanceid = this->rpl_instanceid;
    dio->rpl_dagrank    = this->rpl_dagrank;
    memcpy(dio->rpl_dagid, this->rpl_dagid, 16);
    
    len = ((caddr_t)&dio[1] - (caddr_t)buff);
    
    return len;
}

int network_interface::if_count(void)
{
    class network_interface *iface = network_interface::all_if;
    int count = 0;

    while(iface != NULL) {
        count++;
        iface = iface->next;
    }
    return count;
}

void network_interface::main_loop(FILE *verbose)
{
    bool done = false;

    while(!done) {
        struct pollfd            poll_if[network_interface::if_count()];
        class network_interface* all_if[network_interface::if_count()];
        int pollnum=0;
        
        /*
         * do not really need to build this every time, but, for
         * now, this is fine.
         */
        class network_interface *iface = network_interface::all_if;
        while(iface != NULL) {
            poll_if[pollnum].fd = iface->nd_socket;
            poll_if[pollnum].events = POLLIN;
            poll_if[pollnum].revents= 0;
            all_if[pollnum] = iface;
            
            pollnum++;
            iface = iface->next;
        }

        /* now poll for input */
        int n = poll(poll_if, pollnum, 60 * 1000);
        
        if(n == 0) {
            /* there was a timeout */
        } else if(n > 0) {
            /* there is data ready */
            for(int i=0; i < pollnum && n > 0; i++) {
                if(verbose) fprintf(verbose, "checking source %u -> %s\n",
                                    i,
                                    poll_if[i].revents & POLLIN ? "ready" : "no-data");
                                    
                if(poll_if[i].revents & POLLIN) {
                    all_if[i]->receive();
                    n--;
                }
            }
        } else {
            /* there was an error */
            perror("sunshine poll");
        }
    }
}



/*
 * Local Variables:
 * c-basic-offset:4
 * c-style: whitesmith
 * End:
 */