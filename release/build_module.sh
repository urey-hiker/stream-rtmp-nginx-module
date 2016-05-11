#!/bin/bash

#check rpm
file_aio=""
if [ "$(uname)" == "Linux" ]; then
	file_aio="--with-file-aio"
	if ! rpm -q --quiet openssl-devel; then
		yum -y install openssl-devel
	fi

	if ! rpm -q --quiet pcre-devel; then
		yum -y install pcre-devel
	fi

	if ! rpm -q --quiet zlib-devel; then
		yum -y install zlib-devel
	fi
fi

if [ "$(uname)" == "Darwin" ]; then
	brew link openssl --force
fi

version=`ls ../nginx-1.* | awk -F\- '{print $2}' | awk -F'.tar.gz' '{print $1}'`
module=$(cd ../ && pwd)
nginx_path=$(cd ../nginx-$version && pwd)

if [ ! -f $nginx-path/Makefile ]; then
	cd $nginx_path
	FLAGS="-g -O0" ./configure \
        --prefix=/etc/nginx \
        --sbin-path=/usr/sbin/nginx \
        --modules-path=/usr/lib64/nginx/modules \
        --conf-path=/etc/nginx/nginx.conf \
        --error-log-path=/var/log/nginx/error.log \
        --http-log-path=/var/log/nginx/access.log \
        --pid-path=/var/run/nginx.pid \
        --lock-path=/var/run/nginx.lock \
        --http-client-body-temp-path=/var/cache/nginx/client_temp \
        --http-proxy-temp-path=/var/cache/nginx/proxy_temp \
        --http-fastcgi-temp-path=/var/cache/nginx/fastcgi_temp \
        --http-uwsgi-temp-path=/var/cache/nginx/uwsgi_temp \
        --http-scgi-temp-path=/var/cache/nginx/scgi_temp \
        --user=nginx \
        --group=nginx \
        --with-http_ssl_module \
        --with-http_realip_module \
        --with-http_addition_module \
        --with-http_sub_module \
        --with-http_dav_module \
        --with-http_flv_module \
        --with-http_mp4_module \
        --with-http_gunzip_module \
        --with-http_gzip_static_module \
        --with-http_random_index_module \
        --with-http_secure_link_module \
        --with-http_stub_status_module \
        --with-http_auth_request_module \
        --with-threads \
        --with-stream \
        --with-stream_ssl_module \
        --with-http_slice_module \
        --with-mail \
        --with-mail_ssl_module \
	${file_aio} \
        --with-ipv6 \
        --with-http_v2_module \
        --with-debug --add-dynamic-module=$module
	if [ $? != 0 ]; then
		rm -rf Makefile
	fi
	cd -
fi

make -C $nginx_path modules

if [ -f $nginx_path/objs/ngx_stream_rtmp_module.so ]; then
	mv -f $nginx_path/objs/ngx_stream_rtmp_module.so ./
	echo build modules Done...
fi


