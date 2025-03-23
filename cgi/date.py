# date.py

import datetime

# 現在の日付と時刻を取得
current_datetime = datetime.datetime.now()

# フォーマットを指定して出力（例: 2025-03-19 12:34:56）
print("現在の日時:", current_datetime.strftime("%Y-%m-%d %H:%M:%S"))
