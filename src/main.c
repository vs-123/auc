#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "uiohook.h"

volatile bool is_clicking = false;
volatile bool is_running  = true;

void* clicker_thread(void* arg)
{
   while (is_running) {
      if (is_clicking) {
         uiohook_event event     = { .type = EVENT_MOUSE_PRESSED };
         event.data.mouse.button = 1;
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
   if (event->type == EVENT_KEY_PRESSED) {
      if (event->data.keyboard.keycode == VC_F5) {
         is_clicking = !is_clicking;
         printf("[STATUS] %s\n", (is_clicking) ? "CLICKING NOW" : "STOPPED CLICKING");
         fflush(stdout);
      }

      if (event->data.keyboard.keycode == VC_ESCAPE) {
         printf("[STATUS] EXITING...");
         is_running = false;
         hook_stop();
      }
   }
}

int main(void)
{
   printf("#########################\n");
   printf("#  AUC -- AUTO-CLICKER  #\n");
   printf("#########################\n");
   printf("\n");
   printf("* PRESS F5 TO TOGGLE.\n");

   pthread_t thread_id;
   pthread_create(&thread_id, NULL, clicker_thread, NULL);

   hook_set_dispatch_proc(listener);
   hook_run();

   is_running = false;
   pthread_join(thread_id, NULL);

   return 0;
}
