Full installation procedure for system that are comming without compilation chaine.

```
sudo apt install git make build-essential libssl-dev
cd /tmp
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c/
make
sudo make install
cd /tmp
git clone https://github.com/destroyedlolo/PubSysFigures.git
cd PubSysFigures
gcc -Wall PubSysFigures.c -lpaho-mqtt3c -o PubSysFigures
sudo cp PubSysFigures /usr/local/sbin
```

Then create an unprevilegied account and then following **/etc/systemd/system/PubSysFigures.service**

```
[Unit]
Description=Publish system figures to MQTT
After=network.target

[Service]
Type=simple
User=youraccount
Group=youraccount
ExecStart=/usr/local/sbin/PubSysFigures -Htorchwood.local -s30 -g120
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

And finally

```
sudo systemctl daemon-reload
sudo update-rc.d PubSysFigures defaults
sudo service PubSysFigures start
```
