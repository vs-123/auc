#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "uiohook.h"

typedef enum {
   MOUSE_BTN_UNSET = 0,
   MOUSE_BTN_LEFT  = 1,
   MOUSE_BTN_RIGHT = 2,
} mouse_btn_t;

typedef struct {
   mouse_btn_t mouse_btn;
   unsigned int interval;
} auc_t;

volatile int mouse_x      = 0;
volatile int mouse_y      = 0;
volatile bool is_clicking = false;
volatile bool is_running  = true;
auc_t settings            = { .mouse_btn = MOUSE_BTN_UNSET, .interval = 100 };

void* clicker_thread(void* arg)
{
   auc_t* cfg = (auc_t*)arg;
   while (is_running) {
      if (is_clicking) {
         uiohook_event event     = { .type = EVENT_MOUSE_PRESSED };
         event.data.mouse.button = (uint16_t)cfg->mouse_btn;
         event.data.mouse.x      = (int16_t)mouse_x;
         event.data.mouse.y      = (int16_t)mouse_y;

         hook_post_event(&event);
         event.type = EVENT_MOUSE_RELEASED;
         hook_post_event(&event);

         usleep(cfg->interval * 1000);
      } else {
         usleep(50000);
      }
   }
   return NULL;
}

void listener(uiohook_event* const event)
{
   switch (event->type) {
   case EVENT_MOUSE_MOVED:
   case EVENT_MOUSE_DRAGGED:
      mouse_x = event->data.mouse.x;
      mouse_y = event->data.mouse.y;
      break;
   case EVENT_KEY_PRESSED:
      if (event->data.keyboard.keycode == VC_F6) {
         is_clicking = !is_clicking;
         printf("[STATUS] %s\n", (is_clicking) ? "CLICKING NOW" : "STOPPED CLICKING");
         fflush(stdout);
      }
      if (event->data.keyboard.keycode == VC_ESCAPE) {
         printf("[STATUS] EXITING...\n");
         is_running = false;
         hook_stop();
      }
      break;
   }
}

void run_auc(auc_t* auc)
{
   pthread_t thread_id;
   pthread_create(&thread_id, NULL, clicker_thread, auc);
   hook_set_dispatch_proc(listener);
   hook_run();
   is_running = false;
   pthread_join(thread_id, NULL);
}

void
print_usage (const char *program_name)
{
#define popt(opt, desc) printf("    %-45s %s\n", opt, desc)
   printf("[USAGE] %s [FLAGS] <INTERVAL_MS>\n", program_name);
   printf("[DESC.] TODO\n");
   printf("[FLAGS]\n");
   popt("--HELP, -H, -?", "PRINT THIS HELP MESSAGE AND EXIT");
   popt("--INFO, -I", "PRINT INFORMATION ABOUT THIS PROGRAM AND EXIT");
   popt("--LEFT, -L", "USE LEFT MOUSE BUTTON");
   popt("--RIGHT, -R", "USE RIGHT MOUSE BUTTON");
   printf("[NOTE] FLAGS ARE NOT CASE-SENSITIVE\n");
#undef popt
}

void print_info(void)
{
#define pinfo(aspect, detail) printf("    * %-17s %s\n", aspect, detail)
   printf("[INFO]\n");
   printf("    AUC -- TODO\n\n");
   pinfo("[AUTHOR]", "vs-123 @ https://github.com/vs-123");
   pinfo("[REPOSITORY]", "https://github.com/vs-123/auc");
   pinfo("[LICENSE]", "GNU AFFERO GENERAL PUBLIC LICENSE VERSION 3.0 OR LATER");
#undef pinfo
}

typedef enum { FLAG_NONE, FLAG_HELP, FLAG_INFO, FLAG_LEFT, FLAG_RIGHT, FLAG_UNKNOWN } flag_type_t;

typedef struct {
   const char* short_flag;
   const char* long_flag;
   flag_type_t type;
} flag_map_t;

const flag_map_t flag_table[] = { { "-h", "--help", FLAG_HELP }, { "-i", "--info", FLAG_INFO },
   { "-l", "--left", FLAG_LEFT }, { "-r", "--right", FLAG_RIGHT } };

int strcmpci(char const* str1, char const* str2)
{
   for (;;) {
      int cmp = tolower((unsigned char)*str1) - tolower((unsigned char)*str2);
      if (*str1 == '\0' || cmp != 0)
         return cmp;
      str1++, str2++;
   }
}

flag_type_t get_flag(const char* arg)
{
   for (size_t i = 0; i < sizeof(flag_table) / sizeof(flag_map_t); i++) {
      if (strcmpci(arg, flag_table[i].short_flag) == 0
          || strcmpci(arg, flag_table[i].long_flag) == 0) {
         return flag_table[i].type;
      }
   }
   return FLAG_UNKNOWN;
}

int parse_arguments(int argc, char** argv, auc_t* settings)
{
   for (int i = 1; i < argc; i++) {
      if (argv[i][0] == '-') {
         flag_type_t type = get_flag(argv[i]);
         switch (type) {
         case FLAG_HELP:
            print_usage(argv[0]);
            return 0;
         case FLAG_INFO:
            print_info();
            return 0;
         case FLAG_LEFT:
            settings->mouse_btn = MOUSE_BTN_LEFT;
            break;
         case FLAG_RIGHT:
            settings->mouse_btn = MOUSE_BTN_RIGHT;
            break;
         default:
            fprintf(stderr, "[ERROR] UNRECOGNISED FLAG '%s', USE --HELP\n", argv[i]);
            return -1;
         }
      } else {
         settings->interval = (unsigned int)atoi(argv[i]);
      }
   }

   if (settings->mouse_btn == MOUSE_BTN_UNSET) {
      fprintf(stderr, "[ERROR] MOUSE BUTTON NOT SPECIFIED, USE --HELP\n");
      return -1;
   }

   return 1;
}

int main(int argc, char** argv)
{
   auc_t settings = {
      .mouse_btn = MOUSE_BTN_UNSET,
      .interval  = 100,
   };
   int result = parse_arguments(argc, argv, &settings);
   if (result <= 0)
      return (result == 0) ? 0 : 1;
   run_auc(&settings);
   return 0;
}
