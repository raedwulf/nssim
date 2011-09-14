/**
 * Copyright (C) 2011 The University of York
 * Author(s):
 *   Tai Chi Minh Ralph Eastwood <tcmreastwood@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * \brief Communication with the PlayerNSD daemon through pipes
 * \author Tai Chi Minh Ralph Eastwood
 * \author University of York
 */

#ifndef _PLAYERNSD_COMMUNICATION_H_
#define _PLAYERNSD_COMMUNICATION_H_

#include <boost/thread/mutex.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstdio>
#include <termios.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

using namespace ns3;

class PlayerNSDCommunication
{
   public:
      /**
       * Constructor for the PlayerNSD communication class.
       * \param The input file path (for a pipe). If 0, then defaults to stdin.
       * \param The output file path (for a pipe). If 0, then defaults to stdin.
       */
      PlayerNSDCommunication(bool verbose = false, char *inputFile = 0, char *outputFile = 0)
      {
         int sin, sout;
         sin = fileno(stdin);
         sout = fileno(stdout);

         // Support verbose logging to stderr
         this->verbose = verbose;

         // Open the inputFile and outputFile
         if (inputFile == 0)
            input = sin;
         else
            input = open(inputFile, O_RDONLY | O_NONBLOCK);
         if (outputFile == 0)
            output = sout;
         else
            output = open(outputFile, O_WRONLY | O_NONBLOCK);

         // If the input is stdin, disable line buffering.
         if (input == sin)
         {
            tcgetattr(sin, &oldSettings);
            newSettings = oldSettings;
            newSettings.c_lflag &= (~ICANON & ~ECHO);
            tcsetattr(sin, TCSANOW, &newSettings);
         }
      }

      /**
       * Destructor for the PlayerNSD communication class.
       */
      ~PlayerNSDCommunication()
      {
         // Return the terminal to its original state.
         tcsetattr(fileno(stdin), TCSANOW, &oldSettings);
      }

      /**
       * Set properties.
       * \param variable Variable name of the property.
       * \param value Value of the property.
       */
      void SetProperty(const char *variable, const char *value)
      {
         properties[std::string(variable)] = value;
      }

      /**
       * Get property value.
       * \param variable Variable name oft he property.
       */
      const char *GetProperty(const char *variable)
      {
         return properties[std::string(variable)].c_str();
      }

   private:
      /**
       * Packet type for IPC.
       */
      enum PacketType
      {
         PT_NEW_CLIENT = 0,
         PT_SEND = 1,
         PT_RECV = 2,
         PT_DISCONNECT = 3,
         PT_PROPGET = 4,
         PT_PROPSET = 5,
         PT_PROPVAL = 6
      };

      // Mutex to preserve atomicity for writing.
      boost::mutex writeMutex;

      // Terminal settings for stdin/stdout.
      struct termios oldSettings, newSettings;
      // fileno for input & output streams.
      int input, output;

      // Property maps.
      std::map<std::string, std::string> properties;

      /**
       * Function to check if there any more input.
       * \return Returns whether there's any more input.
       */
      bool hasInput()
      {
         struct timeval tv = { 0L, 0L };
         fd_set fds;
         FD_ZERO(&fds);
         FD_SET(input, &fds);
         return select(1, &fds, NULL, NULL, &tv);
      }

   protected:
      // Verbose logging
      bool verbose;

      /**
       * Atomic write of the PT_RECV message to the output.
       * \param from The source id of the message.
       * \param to The destination id of the message.
       * \param size The size of the message.
       * \param buffer The message contents.
       */
      void writeRecv(uint32_t from, uint32_t to, uint32_t size, const char *buffer)
      {
         boost::mutex::scoped_lock m(writeMutex);
         char c = PT_RECV;
         write(output, &c, sizeof(char));
         write(output, &from, sizeof(uint32_t));
         write(output, &to, sizeof(uint32_t));
         write(output, &size, sizeof(uint32_t));
         write(output, buffer, size);
         fsync(output);
         if (verbose)
            std::cerr << "RECV: " << from << "->" << to << " (" << size << ") " << std::endl;
      }

      /**
       * Atomic write of the PT_PROPVAL message to the output.
       * \param variable Property variable.name.
       * \param value Property value.
       */
      void writePropVal(uint32_t from, const char *variable, const char *value)
      {
         boost::mutex::scoped_lock m(writeMutex);
         char c = PT_PROPVAL;
         uint32_t varlen = strlen(variable) + 1;
         uint32_t vallen = strlen(value) + 1;
         uint32_t len = varlen + vallen;
         write(output, &c, sizeof(char));
         write(output, &from, sizeof(uint32_t));
         write(output, &len, sizeof(uint32_t));
         write(output, variable, varlen);
         write(output, value, vallen);
         fsync(output);
         if (verbose)
            std::cerr << "PROPVAL: " << from  << " (" << len << ") " << variable << "|" << value << std::endl;
      }

            /**
       * Atomic write of the PT_PROPVAL message to the output.
       * \param socket The socket to disconnect.
       */
      void writeDisconnect(uint32_t socket)
      {
         boost::mutex::scoped_lock m(writeMutex);
         char c = PT_DISCONNECT;
         write(output, &c, sizeof(char));
         write(output, &socket, sizeof(uint32_t));
         fsync(output);
         if (verbose)
            std::cerr << "DISCONNECT: " << socket << std::endl;
      }

      /**
       * Send message in simulation.
       * This function should be overidden in order to pass messages into the
       * simulation.
       * \param from The source id of the message.
       * \param to The destination id of the message.
       * \param size The size of the message.
       * \param buffer The message contents.
       */
      virtual void sendMessage(uint32_t from, uint32_t to, uint32_t size, const char *buffer)
      {
      }

      /**
       * Close a socket.
       * This function should be overriden in order to close sockets on the
       * disconnect message.
       * \param socket The socket ID.
       */
      virtual void closeSocket(uint32_t socket)
      {
      }

      /**
       * Process commands from the pipe/stdin/stdout.
       */
      void processInput()
      {
         char c;
         uint8_t *data;
         uint32_t length;
         const char *var, *value;
         if (hasInput())
         {
            int ret = read(input, &c, 1);
            if (ret < 0)
               std::cerr << "ERROR: Bad read() on input." << std::endl;
            else if (!ret)
               return;

            switch(c)
            {
            case PT_SEND:
               uint32_t from, to;
               // Read the fields.
               read(input, &from, sizeof(uint32_t));
               read(input, &to, sizeof(uint32_t));
               read(input, &length, sizeof(uint32_t));
               // Get the data.
               data = new uint8_t[length];
               read(input, data, length);
               // Set the message.
               sendMessage(from, to, length, (const char *)data);
               delete[] data;
               if (verbose)
                  std::cerr << "SEND: " << from << "->" << to << " (" << length << ") " << std::endl;
               break;
            case PT_DISCONNECT:
               uint32_t socket;
               // Read the socket id.
               read(input, &socket, sizeof(uint32_t));
               // Close the socket.
               closeSocket(socket);
               // Send to the simulation when disconnect has been done...

               break;
            case PT_PROPGET:
               // Read who's requesting this property.
               read(input, &from, sizeof(uint32_t));
               // Read length of message.
               read(input, &length, sizeof(uint32_t));
               // Allocate memory for rest of message.
               data = new uint8_t[length];
               // Get property variable & value.
               read(input, data, length);
               // Property variable.
               var = (char *)data;
               // Get property value.
               value = GetProperty(var);
               if (verbose)
                  std::cerr << "PROPGET: " << from << " (" << length <<") " << var << "|" << value << std::endl;
               // Write value of property.
               writePropVal(from, var, value);
               // Free memory for data.
               delete[] data;
               break;
            case PT_PROPSET:
               // Read length of message.
               read(input, &length, sizeof(uint32_t));
               // Allocate memory for rest of message.
               data = new uint8_t[length];
               // Get property variable & value.
               read(input, data, length);
               // Property variable.
               var = (char *)data;
               // Property value.
               value = ((char *)data) + strlen(var) + 1;
               if (verbose)
                  std::cerr << "PROPSET: " << " (" << length <<") " << var << "|" << value << std::endl;
               // Set value of property.
               SetProperty(var, value);
               // Free memory for data.
               delete[] data;
               break;
            default:
               std::cerr << "ERROR: Unexpected command " << (int)c << std::endl;
            }
         }
      }
};

#endif
