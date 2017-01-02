# 行情数据接收模块

## 模块功能:
1. 通过CThostFtdcMdSpi接口订阅交易所行情  
2. 过滤无效的(如在交易时间外的, 或数据有错误的)行情消息  
3. 将行情信息经由D-Bus发送给量化策略模块  
4. 如果需要, 将行情数据保存到文件  

## 配置文件示例:
文件名: market_watcher.ini  
<pre><code>
[General]  
FlowPath=E:/m/  # 存贮行情接口在本地生成的流文件的目录  

[SubscribeList] # 订阅合约列表, 值为1表示有效  
cu1702=0  
cu1703=1  
al1702=0  
al1703=1  
i1705=1  
T1703=1  
ag1706=1  
rb1705=1  
CF705=1  

[FrontSites]    # 前置机网络地址  
Site1=180.168.146.187:10010
Site2=180.168.146.187:10011

[AccountInfo]   # 帐户信息 (行情端不验证用户名密码, 可以瞎填)  
BrokerID=9999
UserID=12345678  
Password=123456  
</code></pre>
