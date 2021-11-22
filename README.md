# PubSysFigures
Publish unix system figure as MQTT topics

# retained published topics

These topics are published once at daemon starting with the hardware configuration of the host.

* **.../ncpu** : number of cores

# published topics

* **.../Load/1** : loadaverage at 1 minutes
* **.../Load/5** : loadaverage at 5 minutes
* **.../Load/10** : loadaverage at 10 minutes

# command line arguments

**PubSysFigures** accepts options from it's command line.
List of options and short explanations can be obtained with **-h**

```
$ ./PubSysFigures -h
PubSysFigures (1.3)
Publish unix system figures
Known options are :
	-h : this online help
	-H : MQTT server hostname (default "localhost")
	-p : MQTT server port number (default 1883)
	-i : MQTT client identifier (default "psf_<hostname>")
	-t : MQTT topic root (default "Machines/<hostname>")
	-s : delay between sample (default 60s)
	-g : grace periode before broker connected (default : none)
	-v : verbose mode
```
