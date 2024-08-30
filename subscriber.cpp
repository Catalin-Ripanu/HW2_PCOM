#include "messages_TCP_UDP.h"
#include "functions.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <math.h>
#include <iostream>
using namespace std;

/* Functie care indica modul de utilizare */
void usage (char *file)
{
  fprintf (stderr, "usage: %s ID IP_ADDR PORT\n", file);
  exit (0);
}

int main (int argc, char **argv)
{
  if (argc != 4)
    {
      usage (argv[0]);
    }
	/* Structuri necesare pentru ceea ce va urma */
  setvbuf (stdout, NULL, _IONBF, BUFSIZ);
  int ret;
  int MAX_fds = -INT16_MAX;
  fd_set clients_FDS;
  FD_ZERO (&clients_FDS);
  struct sockaddr_in client_addr;
  size_t length, count;
  client_addr.sin_family = AF_INET;
  memset (client_addr.sin_zero, 0, 8);
  client_addr.sin_addr =
  {
  parse_addr_str (argv[2])};
  client_addr.sin_port = parse_port_str (argv[3]);
  char *cl_id = proc_cl_id (argv[1]);
  int client_fd = socket (AF_INET, SOCK_STREAM, 0);
  DIE (client_fd < 0, "socket");
  ret =
    setsockopt (client_fd, IPPROTO_TCP, TCP_NODELAY, &(ret = 1),
		sizeof (ret));
  DIE (ret < 0, "setsockopt");
  ret =
    connect (client_fd, (struct sockaddr *) &client_addr,
	     sizeof (client_addr));
  DIE (ret < 0, "connect");
  FD_SET (client_fd, &clients_FDS);
  MAX_fds = max (MAX_fds, client_fd);
  FD_SET (STDIN_FILENO, &clients_FDS);
  MAX_fds = max (MAX_fds, STDIN_FILENO);
  union buffer message;
  /* Se formeaza un mesaj pentru server (contine id-ul clientului) */
  memcpy (message.message_tcp.data, cl_id, strlen (cl_id) + 1);
  length = strlen (cl_id) + 1;
  message.message_tcp.header.len = htons (length);
  message.message_tcp.header.type = message_tcp::type::ID_CLIENT;
  count = 0;
  length += sizeof (message.message_tcp.header);
  /* Se transmite mesajul */
  while (count != length)
    {
      if (count < length)
	{
	  ret = send (client_fd, &message.buffer[count], length - count, 0);
	  if (ret < 0)
	    {
	      goto exit;
	    }
	  count = count + ret;
	}
      else
	break;
    }

  while (1)
    {
      fd_set clients_FDS_copy = clients_FDS;
      ret = select (MAX_fds + 1, &clients_FDS_copy, NULL, NULL, NULL);
      DIE (ret < 0, "select");
      if (FD_ISSET (client_fd, &clients_FDS_copy))
	{
	  length = sizeof (message.message_tcp.header);
	  count = 0;
	  bool header = false;
	  while (count < length)
	    {
	      ret =
		recv (client_fd, &message.buffer[count], length - count, 0);
	      if (ret <= 0)
		{
		  goto exit;
		}
	      count += ret;
	      if (!header && count >= length)
		{

		  length = length + ntohs (message.message_tcp.header.len);
		  header = true;
		}
	    }
	  message.message_tcp.header.len =
	    ntohs (message.message_tcp.header.len);
		/* Se analizeaza antetul mesajului curent */
	  switch (message.message_tcp.header.type)
	    {
			/* Un mesaj primit din partea server-ului (UDP) */
	    case message_tcp::type::INFO_SV:
	      {
		char topic[MESSAGE_UDP_TOPIC + 1] = { };
		strncpy (topic, message.message_udp.message.topic,
			 MESSAGE_UDP_TOPIC);
		cout << inet_ntoa (message.message_tcp.header.cl_dgram_addr.
				   sin_addr) << ":" << ntohs (message.
							      message_tcp.
							      header.
							      cl_dgram_addr.
							      sin_port) <<
		  " - " << topic << " - " << message_udp::str_type[message.
								   message_udp.
								   message.
								   type] <<
		  " - ";
		count = 0;
       /* Se analizeaza toate tipurile de mesaje UDP */
		switch (message.message_udp.message.type)
		  {
		  case message_udp::type::STRING:
		    {
		      char str[MESSAGE_UDP_DATA + 1];
		      strncpy (str, &message.message_udp.message.data[count],
			       MESSAGE_UDP_DATA);

		      cout << str;

		      break;
		    }

		  case message_udp::type::SHORT_REAL:
		    {
		      uint16_t value;
		      memcpy (&value,
			      &message.message_udp.message.data[count],
			      sizeof (value));
			/* Se proceseaza formatul */
		      printf ("%u.%02u", ntohs (value) / 100,
			      ntohs (value) % 100);
		      break;
		    }

		  case message_udp::type::FLOAT:
		    {
		      uint8_t size;
		      uint32_t value;
		      uint8_t extend;
		      memcpy (&size, &message.message_udp.message.data[count],
			      sizeof (size));
		      count = count + sizeof (size);

		      memcpy (&value,
			      &message.message_udp.message.data[count],
			      sizeof (value));
		      count = count + sizeof (value);

		      memcpy (&extend,
			      &message.message_udp.message.data[count],
			      sizeof (extend));
		      count = count + sizeof (extend);
			/* Se proceseaza formatul */
		      if (extend == 0)
			{
			  if (size)
			    cout << "-" << ntohl (value);
			  else
			    cout << ntohl (value);
			}
		      else
			{
			  long power = pow (10, extend);
			/* Se afiseaza corespunzator */
			  if (size)
			    printf ("-%u.%0*u", (int) (ntohl (value) / power),
				    extend, (int) (ntohl (value) % power));
			  else
			    printf ("%u.%0*u", (int) (ntohl (value) / power),
				    extend, (int) (ntohl (value) % power));
			}

		      break;
		    }

		  case message_udp::type::INT:
		    {
			/* Se proceseaza formatul */
		      uint8_t size;
		      memcpy (&size, &message.message_udp.message.data[count],
			      sizeof (size));
		      count += sizeof (size);
		      int value;
		      memcpy (&value,
			      &message.message_udp.message.data[count],
			      sizeof (value));
		      if (size)
			cout << "-" << ntohl (value);
		      else
			cout << ntohl (value);
		      break;
		    }

		  default:
		    fprintf (stderr, "Strange message received from SV.\n");
		    continue;
		  }

		printf ("\n");

		break;
	      }

	    case message_tcp::type::ANSWER_SV:
	      {
		printf ("%s", message.message_tcp.data);
		break;
	      }

	    default:
	      fprintf (stderr, "Strange message received from SV.\n");
	      continue;
	    }
	}

      if (FD_ISSET (STDIN_FILENO, &clients_FDS_copy))
	{
		/* Se citesc comenzile */
	  char topic[MESSAGE_UDP_TOPIC + 1] = { };
	  char *SF_buffer;
	  char *topic_buff;
	  int size_buff;
	  if (!fgets (message.buffer, sizeof (message.buffer), stdin))
	    {
	      fprintf (stderr, "No command.\n");
	      continue;
	    }
	  size_buff = strlen (message.buffer);
	  if (size_buff > 0 && message.buffer[size_buff - 1] == '\n')
	    message.buffer[--size_buff] = '\0';
	  char *command_buffer = strtok (message.buffer, " ");

	  if (command_buffer == NULL)
	    {
	      fprintf (stderr, "No command.\n");
	      continue;
	    }
		/* Cazul in care se doreste unsubscribe */
	  if (strcmp (command_buffer, "unsubscribe") == 0)
	    {
	      length = 0;
	      char *topic_buff = strtok (NULL, " ");

	      if (topic_buff == NULL)
		{
		  fprintf (stderr, "No topic.\n");
		  continue;
		}
	      strncpy (topic, topic_buff, MESSAGE_UDP_TOPIC);
	      memcpy (&message.message_tcp.data[length], topic,
		      strlen (topic) + 1);
	      length = length + strlen (topic) + 1;

	      message.message_tcp.header.type =
		message_tcp::type::UNSUBSCRIBE;
	      goto process;
	    }
		/* Cazul in care se doreste subscribe */
	  else if (strcmp (command_buffer, "subscribe") == 0)
	    {
	      topic_buff = strtok (NULL, " ");

	      if (topic_buff == NULL)
		{
		  fprintf (stderr, "No topic.\n");
		  continue;
		}
	      strncpy (topic, topic_buff, MESSAGE_UDP_TOPIC);
	      SF_buffer = strtok (NULL, " ");
	      if (SF_buffer == NULL)
		{
		  fprintf (stderr, "No SF.\n");
		  continue;
		}
	      else if (strcmp (SF_buffer, "1") != 0
		       && strcmp (SF_buffer, "0") != 0)
		{
		  fprintf (stderr, "Wrong SF %s.\n", SF_buffer);
		  continue;
		}
	      char number;
	      if (SF_buffer[0] == '1')
		number = SF_buffer[0];
	      length = 0;
	      memcpy (&message.message_tcp.data[length], topic,
		      strlen (topic) + 1);
	      length = length + strlen (topic) + 1;
	      memcpy (&message.message_tcp.data[length], &number,
		      sizeof (number));
	      length = length + sizeof (number);
	      message.message_tcp.header.type = message_tcp::type::SUBSCRIBE;
	      goto process;
	    }
		/* Cazul in care se doreste exit */
	  if (strcmp (command_buffer, "exit") == 0)
	    {
	      ret = shutdown (client_fd, SHUT_WR);
	      DIE (ret < 0, "shutdown");
	      continue;
	    }
	  else
	    {
	      fprintf (stderr, "Wrong command '%s'!.\n", command_buffer);
	      continue;
	    }

	process:
	  message.message_tcp.header.len = htons (length);

	  length += sizeof (message.message_tcp.header);
	  count = 0;
     /* Se trimite mesajul catre server */
	  while (count != length)
	    {
	      if (count < length)
		{
		  ret =
		    send (client_fd, &message.buffer[count], length - count,
			  0);
		  if (ret < 0)
		    {
		      goto exit;
		    }
		  count = count + ret;
		}
	      else
		break;
	    }
	}
    }
  /* Deconectare */
exit:
  ret = close (client_fd);
  DIE (ret < 0, "close");
  puts ("Disconnected!");
  return 0;
}
