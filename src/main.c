#include <ctype.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "uiohook.h"

#define MINIMUM_CLICKS_PER_INTERVAL 1
#define MINIMUM_INTERVAL            100

typedef struct
{
   uint16_t mouse_btn; /* uiohook btn */
   unsigned int interval;
   unsigned int clicks_per_iter;
   atomic_int mouse_x;
   atomic_int mouse_y;
   atomic_bool is_clicking;
   atomic_bool is_running;
} auc_t;

static auc_t *g_ctx = NULL;

void *
clicker_thread (void *arg)
{
   auc_t *cfg = (auc_t *)arg;
   while (cfg->is_running)
      {
         if (cfg->is_clicking)
            {
               uiohook_event event     = { .type = EVENT_MOUSE_PRESSED };
               event.data.mouse.button = (uint16_t)cfg->mouse_btn;
               event.data.mouse.x      = (int16_t)cfg->mouse_x;
               event.data.mouse.y      = (int16_t)cfg->mouse_y;

               hook_post_event (&event);
               event.type = EVENT_MOUSE_RELEASED;
               hook_post_event (&event);

               usleep (cfg->interval * 1000);
            }
         else
            {
               usleep (50000);
            }
      }
   return NULL;
}

void
listener (uiohook_event *const event)
{
   if (!g_ctx)
      {
         return;
      }

   switch (event->type)
      {
      case EVENT_MOUSE_MOVED:
      case EVENT_MOUSE_DRAGGED:
         atomic_store (&g_ctx->mouse_x, event->data.mouse.x);
         atomic_store (&g_ctx->mouse_y, event->data.mouse.y);
         break;
      case EVENT_KEY_PRESSED:
         if (event->data.keyboard.keycode == VC_F6)
            {
               bool is_clicking = atomic_load (&g_ctx->is_clicking);
               atomic_store (&g_ctx->is_clicking, !is_clicking);
               printf ("[STATUS] %s\n", (is_clicking) ? "CLICKING NOW" : "STOPPED CLICKING");
               fflush (stdout);
            }
         if (event->data.keyboard.keycode == VC_ESCAPE)
            {
               printf ("[STATUS] EXITING...\n");
               atomic_store (&g_ctx->is_running, false);
               hook_stop ();
            }
         break;
      default:
         /* NOTHING */
         break;
      }
}

void
run_auc (auc_t *auc)
{
   printf ("[STATUS] AUC STARTED, PRESS F6 TO TOGGLE\n");
   pthread_t thread_id;
   pthread_create (&thread_id, NULL, clicker_thread, auc);
   hook_set_dispatch_proc (listener);
   hook_run ();
   atomic_store (&g_ctx->is_running, false);
   pthread_join (thread_id, NULL);
}

void
print_usage (const char *program_name)
{
#define popt(opt, desc) printf ("    %-45s %s\n", opt, desc)
   printf ("[USAGE] %s [FLAGS] <INTERVAL_MS> <CLICKS_PER_ITER>\n", program_name);
   printf ("[DESC.] TODO\n");
   printf ("[FLAGS]\n");
   popt ("--HELP, -H, -?", "PRINT THIS HELP MESSAGE AND EXIT");
   popt ("--INFO, -I", "PRINT INFORMATION ABOUT THIS PROGRAM AND EXIT");
   popt ("--LEFT, -L", "USE LEFT MOUSE BUTTON");
   popt ("--RIGHT, -R", "USE RIGHT MOUSE BUTTON");
   printf ("[NOTE] FLAGS ARE NOT CASE-SENSITIVE\n");
#undef popt
}

void
print_info (void)
{
#define pinfo(aspect, detail) printf ("    * %-17s %s\n", aspect, detail)
   printf ("[INFO]\n");
   printf ("    AUC -- TODO\n\n");
   pinfo ("[AUTHOR]", "vs-123 @ https://github.com/vs-123");
   pinfo ("[REPOSITORY]", "https://github.com/vs-123/auc");
   pinfo ("[LICENSE]", "GNU AFFERO GENERAL PUBLIC LICENSE VERSION 3.0 OR LATER");
#undef pinfo
}

typedef enum
{
   FLAG_NONE,
   FLAG_HELP,
   FLAG_INFO,
   FLAG_LEFT,
   FLAG_RIGHT,
   FLAG_UNKNOWN
} flag_type_t;

typedef struct
{
   const char *short_flag;
   const char *long_flag;
   flag_type_t type;
} flag_map_t;

const flag_map_t flag_table[] = {
   { "-h", "--help", FLAG_HELP },
   { "-i", "--info", FLAG_INFO },
   { "-l", "--left", FLAG_LEFT },
   { "-r", "--right", FLAG_RIGHT },
};

int
strcmpci (char const *str1, char const *str2)
{
   for (;;)
      {
         int cmp = tolower ((unsigned char)*str1) - tolower ((unsigned char)*str2);
         if (*str1 == '\0' || cmp != 0)
            {
               return cmp;
            }
         str1++, str2++;
      }
}

flag_type_t
get_flag (const char *arg)
{
   for (size_t i = 0; i < sizeof (flag_table) / sizeof (flag_map_t); i++)
      {
         if (strcmpci (arg, flag_table[i].short_flag) == 0
             || strcmpci (arg, flag_table[i].long_flag) == 0)
            {
               return flag_table[i].type;
            }
      }
   return FLAG_UNKNOWN;
}

int
parse_arguments (int argc, char **argv, auc_t *settings)
{
   unsigned int positional_count = 0;

   for (int i = 1; i < argc; i++)
      {
         if (argv[i][0] == '-')
            {
               flag_type_t type = get_flag (argv[i]);
               switch (type)
                  {
                  case FLAG_HELP:
                     print_usage (argv[0]);
                     return 0;
                  case FLAG_INFO:
                     print_info ();
                     return 0;
                  case FLAG_LEFT:
                     settings->mouse_btn = 1;
                     break;
                  case FLAG_RIGHT:
                     settings->mouse_btn = 2;
                     break;
                  default:
                     fprintf (stderr, "[ERROR] UNRECOGNISED FLAG '%s', USE --HELP\n", argv[i]);
                     return -1;
                  }
            }
         else
            {
               char *endptr;
               unsigned long val = strtoul (argv[i], &endptr, 10);

               if (*endptr != '\0' || endptr == argv[i])
                  {
                     fprintf (stderr,
                              "[ERROR] '%s' IS NOT A VALID POSITIVE INTEGER, "
                              "USE --HELP\n",
                              argv[i]);
                     return -1;
                  }

               if (positional_count == 0)
                  {
                     if (val < 100)
                        {
                           fprintf (stderr, "[WARNING]\n");
                           fprintf (stderr, "   * YOU TRIED TO SET INTERVAL TO '%zu'\n", val);
                           fprintf (stderr, "   * IT MAY CAUSE SYSTEM INSTABILITY\n");
                           fprintf (stderr, "   * I HAVE SET IT TO %d\n", MINIMUM_INTERVAL);
                           val = 100;
                        }
                     settings->interval = val;
                  }
               else if (positional_count == 1)
                  {
                     if (val < 1)
                        {
                           fprintf (stderr, "[WARNING]\n");
                           fprintf (stderr, "   * YOU TRIED TO SET CLICKS PER INTERVAL TO '%zu'\n",
                                    val);
                           fprintf (stderr, "   * THIS WOULD NOT MAKE ANY CLICKS\n");
                           fprintf (stderr, "   * I HAVE SET IT TO %d\n",
                                    MINIMUM_CLICKS_PER_INTERVAL);
                           val = 1;
                        }
                     settings->clicks_per_iter = val;
                  }
               else
                  {
                     fprintf (stderr, "[ERROR] TOO MANY ARGUMENTS PROVIDED.\n");
                     return -1;
                  }

               positional_count++;
            }
      }

   if (settings->mouse_btn == 0)
      {
         fprintf (stderr, "[ERROR] MOUSE BUTTON NOT SPECIFIED, USE --HELP\n");
         return -1;
      }

   /* THESE WILL ONLY TRIGGER WHEN FLAG NOT PROVIDED */

   if (settings->interval < MINIMUM_INTERVAL)
      {
         fprintf (stderr, "[ERROR] INTERVAL WAS NOT PROVIDED, USE --HELP\n");
         return -1;
      }

   if (settings->clicks_per_iter < MINIMUM_CLICKS_PER_INTERVAL)
      {
         fprintf (stderr, "[ERROR] CLICKS PER INTERVAL WAS NOT PROVIDED, USE --HELP\n");
         return -1;
      }

   if (settings->clicks_per_iter == 0)
      {
         settings->clicks_per_iter = 1;
      }

   return 1;
}

int
main (int argc, char **argv)
{
   auc_t settings = {
      .mouse_btn       = 0,
      .interval        = 0,
      .clicks_per_iter = 0,
      .is_clicking     = false,
      .is_running      = true,
      .mouse_x         = 0,
      .mouse_y         = 0,
   };

   g_ctx = &settings;

   int result = parse_arguments (argc, argv, &settings);
   if (result <= 0)
      {
         return (result == 0) ? 0 : 1;
      }

   run_auc (&settings);
   return 0;
}
