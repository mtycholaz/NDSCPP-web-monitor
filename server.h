
#include <stdio.h>
#include <stdlib.h>
#include <microhttpd.h>
#include <pthread.h>
#include <string.h>
#include <thread>
#include "global.h"
class WebServer
{
        
  private:
        pthread_t server_thread;
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        struct MHD_Daemon *daemon;
    
        static void * RunServer(void *arg);
        static enum MHD_Result AnswerConnection(void                      * cls, 
                                                struct MHD_Connection     * connection,
                                                const char 			  * url, 
                                                const char 			  * method,
                                                const char 			  * version, 
                                                const char 			  * upload_data,
                                                size_t     			  * upload_data_size, 
                                                void      			 ** con_cls);

  public:
  
      WebServer() 
      {
            pthread_mutex_init(&mutex, NULL);
            pthread_cond_init(&cond, NULL);
      }

      ~WebServer() 
      {
            pthread_mutex_destroy(&mutex);
            pthread_cond_destroy(&cond);
      }
      
      pthread_t Start();
      void Stop();
};
