#!/bin/bash

# HTMLの開始
echo "<html>"
echo "<head><title>POST Request Response</title></head>"
echo "<body>"
echo "<h1>POST Request Received</h1>"

# POSTデータの読み取りと表示
echo "<h2>Received POST Data:</h2>"
echo "<pre>"

# 標準入力からPOSTデータを読み取る
if [ "$REQUEST_METHOD" = "POST" ]; then
    # Content-Lengthからデータ長を取得
    CONTENT_LENGTH=$(echo "$CONTENT_LENGTH" | sed 's/[^0-9]//g')

    # データを読み込む
    if [ -n "$CONTENT_LENGTH" ]; then
        POST_DATA=$(dd bs=1 count=$CONTENT_LENGTH 2>/dev/null)
        echo "$POST_DATA"
    else
        echo "No data received or Content-Length not set."
    fi
else
    echo "This script only handles POST requests."
fi

echo "</pre>"

# 環境変数の表示
echo "<h2>Environment Variables:</h2>"
echo "<pre>"
env | sort
echo "</pre>"

# HTMLの終了
echo "</body>"
echo "</html>"
