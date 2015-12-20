<?php

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
