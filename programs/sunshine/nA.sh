#!/bin/sh

/sbin/sunshine -K; sleep 1
/sbin/sunshine -v --dagid FUNFUNFUN -i eth1 -W 10000 --prefix 2001:1234:5678::/48 --rank 1 -I 1 2>&1 | tee /var/log/nA.log

