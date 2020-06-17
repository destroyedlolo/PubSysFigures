# PubSysFigures
Publish unix system figure as MQTT topics

# retained published topics

These topics are published once at daemon starting with the hardware configuration of the host.

* **.../ncpu** : number of cores

# published topics

* **.../Load/1** : loadaverage at 1 minutes
* **.../Load/5** : loadaverage at 5 minutes
* **.../Load/10** : loadaverage at 10 minutes
