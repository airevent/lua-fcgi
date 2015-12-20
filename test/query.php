<?php // via nginx

/*
nginx config:

upstream lua {
    server 127.0.0.1:12345;
    keepalive 32;
}

server {
    listen [::1]:8090;

    set $root /srv/http;

    charset utf-8;
    root $root;
    index index.html;

    location /lua {
        fastcgi_index index.lua;
        fastcgi_pass lua;
        fastcgi_next_upstream error timeout invalid_header http_500 http_503;
        fastcgi_connect_timeout 2s;
        fastcgi_read_timeout 30s;
        fastcgi_send_timeout 5s;
        fastcgi_keep_conn on;
        fastcgi_ignore_client_abort off;
        fastcgi_intercept_errors on;
        fastcgi_store_access user:rw group:r all:r;
        fastcgi_split_path_info ^(.+\.lua)(.*)$;

        include fastcgi_params;
        fastcgi_param SCRIPT_FILENAME $root$fastcgi_script_name;
        fastcgi_param DOCUMENT_ROOT $root;
    }
}

*/

$url = "http://localhost:8090/lua/op?a=1&b=lol#wtf";
$params = [
    "secret" => "aojfOEFJOE9242658",
    "noob" => "lol",
];

$ctx = stream_context_create(["http" => [
    "method" => "POST",
    "user_agent" => "php5 file_get_contents (with ctx)",
    "timeout" => 1,
    "header" => [
        "Content-Type: application/x-www-form-urlencoded; charset=utf-8",
        "Connection: close",
    ],
    "content" => http_build_query($params),
]]);

$r = file_get_contents($url, false, $ctx);

print_r($r);
