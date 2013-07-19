output.cpp
----------
提交用的bot程序,由src/下的文件生成.
运行```./merge.sh```或```make```会自动生成output.cpp

enclosure_bot_manual.py
-----
一个手工控制的bot

enclosure_judge.py
-----
judge程序,使用```./enclosure_judge.py bot1 bot2 bot3 bot3 [-w | -gui]```运行

> \-gui 显示图形界面

> \-w 把分数追加到score.log中,可用于反复运行

enclosure_analyse.py
-----
```./enclosure_judge.py score.log```分析 score.log 按规则统计得分