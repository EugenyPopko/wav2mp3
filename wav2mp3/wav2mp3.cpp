/**
 *  \file
 *  \author Popko Eugeny
 *  \brief WAV to MP3 converter. Program implemented by using LAME, queue and pthreads.
 */

#ifdef _WIN32
#include <windows.h>
#include "dirent.h"
#define HAVE_STRUCT_TIMESPEC
#else
#include <unistd.h>
#include <dirent.h>
#endif
#include <lame.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <queue> 

using namespace std;

// Working queue of files to convert
queue <string> q;

// condition variable and mutex
pthread_cond_t      cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;

// main thread function for convert
void* Convert2MP3(void* ptr)
{
   for (;;)
   {
      pthread_mutex_lock(&mutex);
      // While queue is empty - wait signal
      while (q.empty())
         pthread_cond_wait(&cond, &mutex);
      string s = q.front();
      // Get Stop command - exit
      if ((s.compare("stop")) == 0)
      {
         pthread_mutex_unlock(&mutex);
         break;
      }
      q.pop();
      pthread_mutex_unlock(&mutex);

      // Form mp3-name
      string mp3_path(s);
      mp3_path.replace(mp3_path.end()-3,mp3_path.end(),"mp3");

      size_t read, write;
      FILE *pcm = fopen(s.c_str(), "rb");  //source
      fseek(pcm, 4*1024, SEEK_CUR);        //skip file header

      FILE *mp3 = fopen(mp3_path.c_str(), "wb");  //output
      const int PCM_SIZE = 8192; 
      const int MP3_SIZE = 8192; 
      short int pcm_buffer[PCM_SIZE*2]; 
      unsigned char mp3_buffer[MP3_SIZE];
 
      // init lame, standard settings 
      lame_t lame = lame_init(); 
      lame_set_VBR(lame, vbr_default); 
      lame_set_num_channels( lame, 2 ); 
      lame_set_in_samplerate( lame, 44100 ); 
      lame_set_brate( lame, 128 ); 
      lame_set_mode( lame, STEREO ); 
      lame_set_quality( lame, 5 );  // good quality, fast 
      lame_init_params(lame);
 
      // convert 
      do {
         read = fread(pcm_buffer, 2*sizeof(short int), PCM_SIZE, pcm);
         if (read == 0)
            write = lame_encode_flush(lame, mp3_buffer, MP3_SIZE);
         else
            write = lame_encode_buffer_interleaved(lame,pcm_buffer, read, mp3_buffer, MP3_SIZE);

         fwrite(mp3_buffer, write, 1, mp3);
      } while (read != 0);

      lame_close(lame);
      fclose(mp3);
      fclose(pcm);
   }

   return 0;
}

pthread_t threads[10];

int main(int argc, char **argv)
{
   // number of cpu cores= number of threads
   unsigned long numthreads = 1;

   // initialize the condition variable and mutex
   pthread_cond_init(&cond, NULL);
   pthread_mutex_init(&mutex, NULL);

   if (argc != 2)
   {
      printf("Error: expected command line argument: path to wav");
      return -1;
   }

   string wav_dir(argv[1]);
   if (wav_dir.empty())
   {
      printf("Error: directory with empty name");
      return -1;
   }

   char lastChar = *wav_dir.rbegin();
   #ifdef _WIN32
   // Correct path's slash
   if (lastChar != '\\')
      wav_dir += "\\";
   // Get number of cores
   SYSTEM_INFO sysinfo;
   GetSystemInfo(&sysinfo);
   numthreads = sysinfo.dwNumberOfProcessors;
   #else
   // Correct path's slash
   if (lastChar != '/')
      wav_dir += "/";
   // Get number of cores
   numthreads = sysconf(_SC_NPROCESSORS_ONLN);
   #endif
   printf("Number of CPU cores: %ld\n", numthreads);

   for (int i = 0; i < numthreads; i++ ){
      pthread_create(&threads[i], NULL, Convert2MP3, NULL);
   }

   DIR *d;
   struct dirent *dir;
   d = opendir(wav_dir.c_str());
   if (d == NULL) {
      printf ("Cannot open directory '%s'\n", wav_dir.c_str());
      return -1;
   }

   while ((dir = readdir(d)) != NULL)
   {
      // Check for extension
      if( strcmp(strrchr(dir->d_name, '.'), ".wav") == 0 )
      {
         // Form path to Wav-file
         string file_name(dir->d_name);
         string full_path = wav_dir + file_name;
         printf("File %s\n", full_path.c_str());
         // Send new data to convert
         pthread_mutex_lock(&mutex);
         q.push(full_path);
         pthread_mutex_unlock(&mutex);
         pthread_cond_signal(&cond);
      }
   }
   closedir(d);

   // Send stop signal
   pthread_mutex_lock(&mutex);
   q.push(string("stop"));
   pthread_mutex_unlock(&mutex);
   pthread_cond_signal(&cond);

   // wait for the threads
   for (int i = 0; i < numthreads; i++)
      pthread_join(threads[i], NULL);

   pthread_mutex_destroy(&mutex);
   pthread_cond_destroy(&cond);

   return(EXIT_SUCCESS);
}
