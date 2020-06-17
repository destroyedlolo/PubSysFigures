/*
 * PubSysFigures
 * 	Publish unix system figures as MQTT topics
 *
 * 	Compilation
 *
gcc -lpthread -lpaho-mqtt3c -Wall PubSysFigures.c -o PubSysFigures
 *
 * Copyright 2015 Laurent Faillie
 *
 *			TeleInfod is covered by 
 *		Creative Commons Attribution-NonCommercial 3.0 License
 *		(http://creativecommons.org/licenses/by-nc/3.0/) 
 *		Consequently, you're free to use if for personal or non-profit usage,
 *		professional or commercial usage REQUIRES a commercial licence.
 *
 *		TeleInfod is distributed in the hope that it will be useful,
 *		but WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	16/06/2020 - v0.1 LF - Start of development
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>		/* basename */

#include <assert.h>

#include <MQTTClient.h>

#define VERSION "0.1"

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


	/*
	 * Let's go
	 */

struct Config {
	const char *Broker_Host;
	int Broker_Port;
	char *id;
	char *topic;
	bool verbose;

	MQTTClient client;
	const char *hostname;
	size_t hnlen;
	size_t topiclen;
} cfg;

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

	char l[MAXLINE];
	FILE *f;

	if(ac > 0){
		int i;
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

	if(cfg.verbose)
		printf("*I* Hostname = \"%s\"\n", cfg.hostname);

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
	cfg.topiclen = strlen(cfg.topic);
	if(cfg.verbose)
		printf("*I* Topics' root = \"%s\"\n", cfg.topic);

}
