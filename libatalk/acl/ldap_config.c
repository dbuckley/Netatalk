/*
   $Id: ldap_config.c,v 1.1 2009-02-02 11:55:01 franklahm Exp $
   Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_NFSv4_ACLS

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <ldap.h>

#include <atalk/ldapconfig.h>
#include <atalk/logger.h>

#define LINESIZE 1024

/* Parse one line. Return result in pref and val */
static int getpref(char *buf, char **R_pref, char **R_val)
{
    char *p, *pref, *val;

    /* a little pre-processing to get rid of spaces and end-of-lines */
    p = buf;
    while (p && isspace(*p))
	p++;
    if (!p || (*p == '\0'))
	return -1;
    
    if ((val = strchr(p, '=')) == NULL)
	return -1;
    while ((*val == '=') || (*val == ' '))
	val++;
    if ((val = strtok(val, " \n")) == NULL)
	return -1;
    if ((val = strdup(val)) == NULL)
	return -1;
    if ((pref = strtok(p, " =")) == NULL)
	return -1;

    *R_pref = pref;
    *R_val = val;
    return 0;
}

/* Parse the ldap.conf file */
int acl_ldap_readconfig(char *name)
{
    int i, j;
    FILE *f;
    char buf[LINESIZE];
    char *pref, *val;

    f = fopen(name,"r");
    if (!f) {
	perror("fopen");
	return -1;
    }

    while (!feof(f)) {
	/* read a line from file */
	if (!fgets(buf, LINESIZE, f) || buf[0] == '#')
	    continue;

	/* parse and return pref and value */
	if ((getpref(buf, &pref, &val)) != 0)
	    continue;

	i = 0;
	/* now see if its a correct pref */
	while(ldap_prefs[i].pref != NULL) {
	    if ((strcmp(ldap_prefs[i].name, pref)) == 0) {
		/* ok, found a valid pref */

		/* check if we have pre-defined values */
		if (0 == ldap_prefs[i].intfromarray) {
		    /* no, its just a string */
		    ldap_prefs[i].valid = 0;
		    if (0 == ldap_prefs[i].strorint)
			/* store string as string */
			*((char **)(ldap_prefs[i].pref)) = val;
		    else
			/* store as int */
			*((int *)(ldap_prefs[i].pref)) = atoi(val);
		} else {
		    /* ok, we have string to int mapping for this pref
		       eg. "none", "simple", "sasl" map to 0, 128, 129 */
		    j = 0;
		    while(prefs_array[j].pref != NULL) {
			if (((strcmp(prefs_array[j].pref, pref)) == 0) &&
			    ((strcmp(prefs_array[j].valuestring, val)) == 0)) {
			    ldap_prefs[i].valid = 0;
			    *((int *)(ldap_prefs[i].pref)) = prefs_array[j].value;
			}
			j++;
		    } /* while j*/
		} /* else */
	    }
	    i++;
	} /* while i */
    }  /*  EOF */

    /* check if the config is sane and complete */
    i = 0;
    ldap_config_valid = 1;

    while(ldap_prefs[i].pref != NULL) {
	if ( ldap_prefs[i].valid != 0) {
	    ldap_config_valid = 0;
	    break;
	}
	i++;
    }

    if (ldap_config_valid) {
	if (ldap_auth_method == LDAP_AUTH_NONE)
	    LOG(log_debug, logtype_afpd,"ldappref: Pref is ok. Using anonymous bind.");
	else if (ldap_auth_method == LDAP_AUTH_SIMPLE)
	    LOG(log_debug, logtype_afpd,"ldappref: Pref is ok. Using simple bind.");
	else {
	    ldap_config_valid = 0;
	    LOG(log_error, logtype_afpd,"ldappref: Pref not ok. SASL not yet supported.");	
	}
    } else
	LOG(log_error, logtype_afpd,"ldappref: Pref is not ok.");	
    fclose(f);
    return 0;
}
#endif