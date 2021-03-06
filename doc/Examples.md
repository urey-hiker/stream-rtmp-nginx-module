### Simple Video-on-Demand

    stream {
        server {
            listen 1935;
            rtmp;
            application vod {
                play /var/flvs;
            }
        }
    }

### Simple live broadcast service

    stream {
        server {
            listen 1935;
            rtmp;
            application live {
                live on;
            }
        }
    }

### Re-translate remote stream

    stream {
        server {
            listen 1935;
            rtmp;
            application tv {
                live on;
                pull rtmp://cdn.example.com:443/programs/main pageUrl=http://www.example.com/index.html name=maintv;
            }
        }
    }

### Re-translate remote stream with HLS support

    stream {
        server {
            listen 1935;
            rtmp;
            application tv {
                live on;

                hls on;
                hls_path /tmp/tv2;
                hls_fragment 15s;

                pull rtmp://tv2.example.com:443/root/new name=tv2;
            }
        }
    }

    http {
        server {
            listen 80;
            location /tv2 {
                alias /tmp/tv2;
            }
        }
    }
### Stream your X screen through RTMP

    ffmpeg -f x11grab -follow_mouse centered -r 25 -s cif -i :0.0 -f flv rtmp://localhost/myapp/screen