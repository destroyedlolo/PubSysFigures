/*
 * PubSysFigures
 * 	Publish unix system figures as MQTT topics
 *
 * 	Compilation
 *
gcc -Wall PubSysFigures.c -lpaho-mqtt3c -o PubSysFigures
 *
 * Copyright 2020-25 Laurent Faillie
 *
 *			PubSysFigures is covered by 
 *		Creative Commons Attribution-NonCommercial 3.0 License
 *		(http://creativecommons.org/licenses/by-nc/3.0/) 
 *		Consequently, you're free to use if for personal or non-profit usage,
 *		professional or commercial usage REQUIRES a commercial licence.
 *
 *		PubSysFigures is distributed in the hope that it will be useful,
 *		but WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	16/06/2020 - v0.1 LF - Start of development
 *	17/06/2020 - v1.0 LF - First workable version
 *	20/06/2020 - v1.1 LF - Use '/sys/devices/system/cpu/present'
 *	01/07/2020 - V1.2 LF - Correcting clientID (arg !)
 *	20/07/2020 - v1.3 LF - Add grace period
 *	02/06/2025 - v1.5 LF - Add memory
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>		/* basename */

#include <assert.h>

#include <MQTTClient.h>

#define VERSION "1.5"

#define MAXLINE 1024	/* Maximum length of a line to be read */
#define BRK_KEEPALIVE 60	/* Keep alive signal to the broker */

	/*
	 * Helpers
	 */

char *removeLF(char *s){
	if(!s)
		return s;

	size_t l=strlen(s);
	if(l && s[--l] == '\n')
		s[l] = 0;
	return s;
}

const char *strKWcmp(const char *s, const char *kw){
	size_t klen = strlen(kw);
	if( strncmp(s,kw,klen) )
		return NULL;
	else
		return s+klen;
}

	/*
	 * Let's go
	 */

struct Config {
	const char *Broker_Host;
	int Broker_Port;
	char *id;
	char *topic;
	bool verbose;
	int sample;
	int grace;

	MQTTClient client;
	const char *hostname;
	size_t hnlen;
} cfg;

	/*
	 * MQTT's
	 */

int msgarrived(void *ctx, char *topic, int tlen, MQTTClient_message *msg){
	if(cfg.verbose)
		printf("*I* Unexpected message arrival (topic : '%s')\n", topic);

	MQTTClient_freeMessage(&msg);
	MQTTClient_free(topic);
	return 1;
}

void connlost(void *ctx, char *cause){
	printf("*W* Broker connection lost due to %s\n", cause);
}

int papub( const char *topic, int length, void *payload, int retained ){	/* Custom wrapper to publish */
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	pubmsg.retained = retained;
	pubmsg.payloadlen = length;
	pubmsg.payload = payload;

	return MQTTClient_publishMessage( cfg.client, topic, &pubmsg, NULL);
}

	/*
	 * Let's go
	 */

void theend(void){
/* Some cleanup 
 */
	MQTTClient_disconnect(cfg.client, 10000);	/* 10s for the grace period */
	MQTTClient_destroy(&cfg.client);
}

int main(int ac, char **av){
	cfg.Broker_Host = "localhost";
	cfg.Broker_Port = 1883;
	cfg.verbose = false;
	cfg.id = NULL;
	cfg.topic = NULL;
	cfg.sample = 60;
	cfg.grace = 0;

	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	conn_opts.reliable = 0;
	char l[MAXLINE], *param;
	FILE *f;
	int i;

	if(ac > 0){
		for(i=1; i<ac; i++){
			if(!strcmp(av[i], "-h")){
				fprintf(stderr, "%s (%s)\n"
					"Publish unix system figures\n"
					"Known options are :\n"
					"\t-h : this online help\n"
					"\t-H : MQTT server hostname (default \"localhost\")\n"
					"\t-p : MQTT server port number (default 1883)\n"
					"\t-i : MQTT client identifier (default \"psf_<hostname>\")\n"
					"\t-t : MQTT topic root (default \"Machines/<hostname>\")\n"
					"\t-s : delay between sample (default 60s)\n"
					"\t-g : grace periode before broker connected (default : none)\n" 
					"\t-v : verbose mode\n",
					basename(av[0]), VERSION
				);
				exit(EXIT_FAILURE);
			} else if(!strncmp(av[i], "-H", 2))
				cfg.Broker_Host = av[i] + 2;
			else if(!strncmp(av[i], "-p", 2))
				cfg.Broker_Port = atoi( av[i] + 2 );
			else if(!strncmp(av[i], "-i", 2))
				cfg.id = av[i] + 2;
			else if(!strncmp(av[i], "-t", 2))
				cfg.topic = av[i] + 2;
			else if(!strncmp(av[i], "-s", 2))
				cfg.sample = atoi(av[i] + 2);
			else if(!strncmp(av[i], "-g", 2))
				cfg.grace = atoi(av[i] + 2);
			else if(!strncmp(av[i], "-v", 2))
				cfg.verbose = true;
		}
	}

	/* Determine hostname */
	if(!(f=fopen("/proc/sys/kernel/hostname", "r"))){
		perror("/proc/sys/kernel/hostname");
		exit(EXIT_FAILURE);
	}

	if(!fgets(l, MAXLINE, f)){
		perror("/proc/sys/kernel/hostname");
		exit(EXIT_FAILURE);
	}

	assert( (cfg.hostname = removeLF(strdup(l))) );
	cfg.hnlen = strlen( cfg.hostname );

	fclose(f);

	if(cfg.verbose){
		printf("*I* Broker = \"%s\" port = %d\n", cfg.hostname, cfg.Broker_Port);
		printf("*I* Hostname = \"%s\"\n", cfg.hostname);
		printf("*I* Delay b/w samples = %d\n", cfg.sample);
		printf("*I* grace period = %ds\n", cfg.grace);
	}

	if(!cfg.id){
		assert( (cfg.id = malloc( 5 + cfg.hnlen)) );	/* 5: psf_ + \0 */
		strcpy( cfg.id, "psf_" );
		strcpy( cfg.id+4, cfg.hostname );
	}
	if(cfg.verbose)
		printf("*I* Client ID = \"%s\"\n", cfg.id);

	if(!cfg.topic){
		assert( (cfg.topic = malloc( 10 + cfg.hnlen)) );	/* 10: Machines + \0 */
		strcpy( cfg.topic, "Machines/" );
		strcpy( cfg.topic+9, cfg.hostname );
	}
//	cfg.topiclen = strlen(cfg.topic);
	if(cfg.verbose)
		printf("*I* Topics' root = \"%s\"\n", cfg.topic);

	MQTTClient_create( &cfg.client, cfg.Broker_Host, cfg.id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	MQTTClient_setCallbacks( cfg.client, NULL, connlost, msgarrived, NULL);

	i = 0;
	do {
		switch( MQTTClient_connect( cfg.client, &conn_opts) ){
		case MQTTCLIENT_SUCCESS : 
			i = 1;
			break;
		case 1 : fputs("Unable to connect : Unacceptable protocol version\n", stderr);
			exit(EXIT_FAILURE);
		case 2 : fputs("Unable to connect : Identifier rejected\n", stderr);
			exit(EXIT_FAILURE);
		case 3 : fputs("Unable to connect : Server unavailable\n", stderr);
			exit(EXIT_FAILURE);
		case 4 : fputs("Unable to connect : Bad user name or password\n", stderr);
			exit(EXIT_FAILURE);
		case 5 : fputs("Unable to connect : Not authorized\n", stderr);
			exit(EXIT_FAILURE);
		default :
			fprintf(stderr, "Unable to connect (%d)\n", cfg.grace);
			sleep(1);
		}
	} while( --cfg.grace && !i );

	if( !cfg.grace && !i )
		exit(EXIT_FAILURE);

	atexit(theend);

	/*
	 * Publish statics'
	 */
	if(!(f=fopen("/sys/devices/system/cpu/present", "r"))){
		perror("/sys/devices/system/cpu/present");
		exit(EXIT_FAILURE);
	}
	fscanf(f, "0-%d", &i);
	i++;
	fclose(f);
	if(cfg.verbose)
		printf("*I* cpus = \"%d\"\n", i);
	sprintf( l, "%s/ncpu", cfg.topic);
	param = l + strlen(l) + 2;
	sprintf( param, "%d", i);
	papub( l, strlen(param), param, true );

	for(;;){
		if(!(f=fopen("/proc/loadavg", "r"))){
			perror("/proc/loadavg");
			exit(EXIT_FAILURE);
		}
		float r1,r5,r10;
		fscanf(f, "%f %f %f", &r1, &r5, &r10);
		fclose(f);
		if(cfg.verbose)
			printf("*I* loadav = %.2f %.2f %.2f\n", r1, r5, r10);

		sprintf( l, "%s/Load/1", cfg.topic);
		param = l + strlen(l) + 2;
		sprintf( param, "%.2f", r1);
		papub( l, strlen(param), param, false );
		sprintf( l, "%s/Load/5", cfg.topic);
		param = l + strlen(l) + 2;
		sprintf( param, "%.2f", r5);
		papub( l, strlen(param), param, false );
		sprintf( l, "%s/Load/10", cfg.topic);
		param = l + strlen(l) + 2;
		sprintf( param, "%.2f", r10);
		papub( l, strlen(param), param, false );

		unsigned long long mem_total = 0, mem_free = 0, swap_total = 0, swap_free = 0;
		if(!(f=fopen("/proc/meminfo", "r"))){
			perror("/proc/meminfo");
			exit(EXIT_FAILURE);
		}
		while(fgets(l, sizeof(l), f)){
			const char *val;
			if((val = strKWcmp(l, "MemTotal:"))){
				mem_total = strtoull(val, NULL, 10);
			} else if((val = strKWcmp(l, "MemFree:")))
				mem_free = strtoull(val, NULL, 10);
			else if((val = strKWcmp(l, "SwapTotal:")))
				swap_total = strtoull(val, NULL, 10);
			else if((val = strKWcmp(l, "SwapFree:")))
				swap_free = strtoull(val, NULL, 10);
		}
		fclose(f);
		if(cfg.verbose)
			printf("*I* mem = %llu / %llu swap = %llu / %llu\n", mem_free, mem_total, swap_free, swap_total);

		sprintf( l, "%s/memory", cfg.topic);
		param = l + strlen(l) + 2;
		sprintf( param, "%llu/%llu", mem_free, mem_total);
		papub( l, strlen(param), param, false );
		if(mem_total){
			sprintf( l, "%s/memoryPRC", cfg.topic);
			param = l + strlen(l) + 2;
			sprintf( param, "%.2f", 100-mem_free*100.0/mem_total);
			papub( l, strlen(param), param, false );
		}

		sprintf( l, "%s/swap", cfg.topic);
		param = l + strlen(l) + 2;
		sprintf( param, "%llu/%llu", swap_free, swap_total);
		papub( l, strlen(param), param, false );
		if(swap_total){
			sprintf( l, "%s/swapPRC", cfg.topic);
			param = l + strlen(l) + 2;
			sprintf( param, "%.2f", 100-swap_free*100.0/swap_total);
			papub( l, strlen(param), param, false );
		}

		sleep( cfg.sample );
	}
	
	exit(EXIT_SUCCESS);
}
