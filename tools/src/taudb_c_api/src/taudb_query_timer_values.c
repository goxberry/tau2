#include "taudb_api.h"
#include "libpq-fe.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

TAUDB_TIMER_VALUE* taudb_query_all_timer_values(PGconn* connection, TAUDB_TRIAL* trial) {
#ifdef TAUDB_DEBUG_DEBUG
  printf("Calling taudb_query_all_timer_values(%p)\n", trial);
#endif
  return taudb_query_timer_values(connection, trial, NULL, NULL, NULL);
}

TAUDB_TIMER_VALUE* taudb_query_timer_values(PGconn* connection, TAUDB_TRIAL* trial, TAUDB_TIMER* timer, TAUDB_THREAD* thread, TAUDB_METRIC* metric) {
#ifdef TAUDB_DEBUG_DEBUG
  printf("Calling taudb_query_timer_values(%p,%p,%p,%p)\n", trial, timer, thread, metric);
#endif
  PGresult *res;
  int nFields;
  int i, j;

  // validate inputs
  if (trial == NULL) {
    fprintf(stderr, "Error: trial parameter null. Please provide a valid trial.\n");
	return NULL;
  }

  /* Start a transaction block */
  res = PQexec(connection, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(connection));
    PQclear(res);
    taudb_exit_nicely(connection);
  }

  /*
   * Should PQclear PGresult whenever it is no longer needed to avoid
   * memory leaks
   */
  PQclear(res);

// select ilp.*, ie.name as event_name, m.name as metric_name from interval_location_profile ilp left outer join interval_event ie on ilp.interval_event = ie.id left outer join metric m on ilp.metric = m.id where ie.trial = 206;

  /*
   * Fetch rows from table_name, the system catalog of databases
   */
  char my_query[1024];
  if (taudb_version == TAUDB_2005_SCHEMA) {
    sprintf(my_query,"DECLARE myportal CURSOR FOR select ilp.*, ie.name as timer_name, m.name as metric_name from interval_location_profile ilp inner join interval_event ie on ilp.interval_event = ie.id left outer join metric m on ilp.metric = m.id");
    char* conjoiner = "where";
    if (trial != NULL) {
      sprintf(my_query,"%s where ie.trial = %d", my_query, trial->id);
      conjoiner = "and";
    } 
    if (timer != NULL) {
      sprintf(my_query,"%s %s ie.id = %d", my_query, conjoiner, timer->id);
      conjoiner = "and";
    }
    if (metric != NULL) {
      if ((strcmp(metric->name, "calls") == 0) ||
          (strcmp(metric->name, "subroutines") == 0)) {
		  // we need just one metric, but from this trial
        sprintf(my_query,"%s %s m.id = (select max(id) from metric where trial = %d)", my_query, conjoiner, trial->id);
      } else {
        sprintf(my_query,"%s %s m.id = %d", my_query, conjoiner, metric->id);
      }
      conjoiner = "and";
    }
    if (thread != NULL) {
      sprintf(my_query,"%s %s node = %d and context = %d and thread = %d", my_query, conjoiner, thread->node_rank, thread->context_rank, thread->thread_rank);
    }
  } else {
    //sprintf(my_query,"DECLARE myportal CURSOR FOR select * from measurement where trial = %d", trial->id);
    fprintf(stderr, "Error: 2012 schema not supported yet.\n");
    return NULL;
  }
#ifdef TAUDB_DEBUG_DEBUG
  printf("%s\n", my_query);
#endif
  res = PQexec(connection, my_query);
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    fprintf(stderr, "DECLARE CURSOR failed: %s", PQerrorMessage(connection));
    PQclear(res);
    taudb_exit_nicely(connection);
  }
  PQclear(res);

  res = PQexec(connection, "FETCH ALL in myportal");
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    fprintf(stderr, "FETCH ALL failed: %s", PQerrorMessage(connection));
    PQclear(res);
    taudb_exit_nicely(connection);
  }

  int nRows = PQntuples(res);
  //TAUDB_TIMER_VALUE* timer_values = taudb_create_timer_values(nRows);
  TAUDB_TIMER_VALUE* timer_values = NULL;
  taudb_numItems = nRows;

  nFields = PQnfields(res);

  /* the rows */
  for (i = 0; i < PQntuples(res); i++)
  {
    int node = 0;
    int context = 0;
    int thread = 0;
	char* metric_str;
	char* timer_str;
    TAUDB_TIMER_VALUE* timer_value = taudb_create_timer_values(1);
    /* the columns */
    for (j = 0; j < nFields; j++) {
      if (strcmp(PQfname(res, j), "id") == 0) {
        timer_value->id = atoi(PQgetvalue(res, i, j));
      } else if (strcmp(PQfname(res, j), "interval_event") == 0) {
        timer_value->timer = atoi(PQgetvalue(res, i, j));
      } else if (strcmp(PQfname(res, j), "node") == 0) {
        node = atoi(PQgetvalue(res, i, j));
      } else if (strcmp(PQfname(res, j), "context") == 0) {
        context = atoi(PQgetvalue(res, i, j));
      } else if (strcmp(PQfname(res, j), "thread") == 0) {
        thread = atoi(PQgetvalue(res, i, j));
      } else if (strcmp(PQfname(res, j), "metric_name") == 0) {
        metric_str = PQgetvalue(res, i, j);
      } else if (strcmp(PQfname(res, j), "timer_name") == 0) {
        timer_str = PQgetvalue(res, i, j);
      } else if (strcmp(PQfname(res, j), "metric") == 0) {
        timer_value->metric = atoi(PQgetvalue(res, i, j));
      } else if (strcmp(PQfname(res, j), "inclusive_percentage") == 0) {
        timer_value->inclusive_percentage = atof(PQgetvalue(res, i, j));
      } else if (strcmp(PQfname(res, j), "exclusive_percentage") == 0) {
        timer_value->exclusive_percentage = atof(PQgetvalue(res, i, j));
      } else if (strcmp(PQfname(res, j), "inclusive") == 0) {
        timer_value->inclusive = atof(PQgetvalue(res, i, j));
      } else if (strcmp(PQfname(res, j), "exclusive") == 0) {
        timer_value->exclusive = atof(PQgetvalue(res, i, j));
      } else if (strcmp(PQfname(res, j), "sum_exclusive_squared") == 0) {
        timer_value->sum_exclusive_squared = atof(PQgetvalue(res, i, j));
      } else if (strcmp(PQfname(res, j), "inclusive_per_call") == 0) {
        // ignore this
        continue;
      } else if (strcmp(PQfname(res, j), "call") == 0) {
        // ignore this
        continue;
      } else if (strcmp(PQfname(res, j), "subroutines") == 0) {
        // ignore this
        continue;
      } else {
        printf("Error: unknown column '%s'\n", PQfname(res, j));
        taudb_exit_nicely(connection);
      }
    } 
    timer_value->thread = (node * (trial->contexts_per_node * trial->threads_per_context)) +
                          (context * (trial->threads_per_context)) + 
                          thread;

    char tmp_thread[100];
	sprintf(tmp_thread, "%d", timer_value->thread);
	timer_value->key = malloc(sizeof(char) * (strlen(tmp_thread) + strlen(metric_str) + strlen(timer_str) + 3));
    sprintf(timer_value->key, "%d:%s:%s", timer_value->thread, timer_str, metric_str);
	HASH_ADD_KEYPTR(hh, timer_values, timer_value->key, strlen(timer_value->key), timer_value);
  }

  PQclear(res);

  /* close the portal ... we don't bother to check for errors ... */
  res = PQexec(connection, "CLOSE myportal");
  PQclear(res);

  /* end the transaction */
  res = PQexec(connection, "END");
  PQclear(res);
  
  return (timer_values);
}

TAUDB_TIMER_VALUE* taudb_get_timer_value(TAUDB_TIMER_VALUE* timer_values, TAUDB_TIMER* timer, TAUDB_THREAD* thread, TAUDB_METRIC* metric) {
#ifdef TAUDB_DEBUG_DEBUG
  printf("Calling taudb_get_timer_value(%p,%p,%p,%p)\n", timer_values, timer, thread, metric);
#endif
  if (timer_values == NULL) {
    fprintf(stderr, "Error: timer_values parameter null. Please provide a valid set of timer_values.\n");
    return NULL;
  }
  if (timer == NULL) {
    fprintf(stderr, "Error: timer parameter null. Please provide a valid timer.\n");
    return NULL;
  }
  if (thread == NULL) {
    fprintf(stderr, "Error: thread parameter null. Please provide a valid thread.\n");
    return NULL;
  }
  if (metric == NULL) {
    fprintf(stderr, "Error: metric parameter null. Please provide a valid metric.\n");
    return NULL;
  }
  char tmp_thread[10];
  sprintf(tmp_thread, "%d", thread->index);
  char *key = calloc((strlen(tmp_thread) + strlen(timer->name) + strlen(metric->name) + 3), sizeof(char));
  sprintf(key, "%d:%s:%s", thread->index, timer->name, metric->name);
  //printf("%s\n", key);

  TAUDB_TIMER_VALUE* timer_value = NULL;
  HASH_FIND_STR(timer_values, key, timer_value);
  return timer_value;
}
