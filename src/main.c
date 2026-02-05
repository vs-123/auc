#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "uiohook.h"

volatile int mouse_x      = 0;
volatile int mouse_y      = 0;
volatile bool is_clicking = false;
volatile bool is_running  = true;

void* clicker_thread(void* arg)
{
   while (is_running) {
      if (is_clicking) {
         uiohook_event event     = { .type = EVENT_MOUSE_PRESSED };
         event.data.mouse.button = 1;
         event.data.mouse.x = mouse_x;
         event.data.mouse.y = mouse_y;
         hook_post_event(&event);
         event.type = EVENT_MOUSE_RELEASED;
         hook_post_event(&event);

         usleep(10000);
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
   case EVENT_MOUSE_DRAGGED: {
      mouse_x = event->data.mouse.x;
      mouse_y = event->data.mouse.y;
   } break;

   case EVENT_KEY_PRESSED: {
      if (event->data.keyboard.keycode == VC_F6) {
         is_clicking = !is_clicking;
         printf("[STATUS] %s\n", (is_clicking) ? "CLICKING NOW" : "STOPPED CLICKING");
         fflush(stdout);
      }

      if (event->data.keyboard.keycode == VC_ESCAPE) {
         printf("[STATUS] EXITING...");
         is_running = false;
         hook_stop();
      }
   } break;

   default: {
      /* NOTHING */
   } break;
   }
}

int main(void)
{
   printf("#########################\n");
   printf("#  AUC -- AUTO-CLICKER  #\n");
   printf("#########################\n");
   printf("\n");
   printf("* AUTHOR: vs-123 @ https://github.com/vs-123\n");
   printf("* PRESS F6 TO TOGGLE.\n");

   pthread_t thread_id;
   pthread_create(&thread_id, NULL, clicker_thread, NULL);

   hook_set_dispatch_proc(listener);
   hook_run();

   is_running = false;
   pthread_join(thread_id, NULL);

   return 0;
}
