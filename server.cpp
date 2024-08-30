#include "messages_TCP_UDP.h"
#include "functions.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <list>
#include <string>
#include <unordered_map>
#include <iostream>
using namespace std;

void usage (char *file)
{
  fprintf (stderr, "usage: %s <port>\n", file);
  exit (0);
}

int main (int argc, char **argv)
{
  if (argc != 2)
    {
      usage (argv[0]);
    }
	/* Structuri necesare pentru ceea ce va urma */
  setvbuf (stdout, NULL, _IONBF, BUFSIZ);
  unordered_map < int, string > clients_IDS;
  unordered_map < string, int >clients_FDS;
  unordered_map < string, unordered_map < string, bool >> clients_topics;
  unordered_map < string, unordered_map < string, bool >> subscribers;
  unordered_map < string, list < union buffer >>save_SFS;
  int ret;
  size_t length, count;
  fd_set server_FDS;
  int server_FDS_max = -INT16_MAX;
  FD_ZERO (&server_FDS);
  struct sockaddr_in SV_address;
  SV_address.sin_family = AF_INET;
  memset (SV_address.sin_zero, 0, 8);
  SV_address.sin_addr =
  {
  INADDR_ANY};
  SV_address.sin_port = parse_port_str (argv[1]);
  int SV_FD_UDP = socket (AF_INET, SOCK_DGRAM, 0);
  DIE (SV_FD_UDP < 0, "socket");
  ret =
    setsockopt (SV_FD_UDP, SOL_SOCKET, SO_REUSEADDR, &(ret = 1),
		sizeof (ret));
  DIE (ret < 0, "setsockopt");
  ret =
    bind (SV_FD_UDP, (struct sockaddr *) &SV_address, sizeof (SV_address));
  DIE (ret < 0, "bind");
  FD_SET (SV_FD_UDP, &server_FDS);
  server_FDS_max = max (server_FDS_max, SV_FD_UDP);
  int SV_FD_TCP = socket (AF_INET, SOCK_STREAM, 0);
  DIE (SV_FD_TCP < 0, "socket");
  /* Se dezactiveaza algoritmul lui Nagle */
  ret =
    setsockopt (SV_FD_TCP, IPPROTO_TCP, TCP_NODELAY, &(ret = 1),
		sizeof (ret));
  DIE (ret < 0, "setsockopt");

  ret =
    setsockopt (SV_FD_TCP, SOL_SOCKET, SO_REUSEADDR, &(ret = 1),
		sizeof (ret));
  DIE (ret < 0, "setsockopt");

  ret =
    bind (SV_FD_TCP, (struct sockaddr *) &SV_address, sizeof (SV_address));
  DIE (ret < 0, "bind");

  ret = listen (SV_FD_TCP, SOMAXCONN);
  DIE (ret < 0, "listen");

  FD_SET (SV_FD_TCP, &server_FDS);
  server_FDS_max = max (server_FDS_max, SV_FD_TCP);

  FD_SET (STDIN_FILENO, &server_FDS);
  server_FDS_max = max (server_FDS_max, STDIN_FILENO);

  union buffer message;

  while (1)
    {
      fd_set server_FDS_copy = server_FDS;
      ret = select (server_FDS_max + 1, &server_FDS_copy, NULL, NULL, NULL);
      DIE (ret < 0, "select");

      if (FD_ISSET (SV_FD_UDP, &server_FDS_copy))
	{
	  struct sockaddr_in client_address_UDP;
	  socklen_t cl_addr_dgram_len = sizeof (client_address_UDP);

	  memset (&message.message_udp.message.data, 0, MESSAGE_UDP_DATA);
     /* Se obtin mesajele TCP */
	  ret =
	    recvfrom (SV_FD_UDP, &message.message_udp.message,
		      sizeof (message.message_udp.message), 0,
		      (struct sockaddr *) &client_address_UDP,
		      &cl_addr_dgram_len);
	  if (ret <= 0)
	    {
	      continue;
	    }

	  char topic[MESSAGE_UDP_TOPIC + 1] = { };
	  strncpy (topic, message.message_udp.message.topic,
		   MESSAGE_UDP_TOPIC);
   /* Se analizeaza mesajele TCP */
	for (auto & iter:clients_topics[topic])
	    {
	      length = 0;
	      string client_ID = iter.first;
	      bool SF_number = iter.second;
	      length = length + sizeof (message.message_udp.message.topic) +
		sizeof (message.message_udp.message.type);
	      if (message.message_udp.message.type ==
		  message_udp::type::STRING)
		{
		  uint16_t str_len =
		    strnlen (message.message_udp.message.data,
			     MESSAGE_UDP_DATA) + 1;

		  if (str_len > MESSAGE_UDP_DATA)
		    {
		      str_len = MESSAGE_UDP_DATA;
		    }
		  length = length + str_len;
		}
	      else
		{
		  length =
		    length +
		    message_udp::type_size[message.message_udp.message.type];
		}
		/* Se formeaza mesajul */
	      message.message_tcp.header.type = message_tcp::type::INFO_SV;
	      message.message_tcp.header.cl_dgram_addr = client_address_UDP;
	      message.message_tcp.header.len = htons (length);

	      if (clients_FDS.count (client_ID))
		{
		  int cl_fd = clients_FDS.at (client_ID);
		  length = length + sizeof (message.message_tcp.header);
		  count = 0;
        /* Se trimite mesajul */
		  for (;count < length;)
		    {
		      ret =
			send (cl_fd, &message.buffer[count], length - count,
			      0);
		      if (ret < 0)
			{
			  clients_IDS.erase (cl_fd);
			  clients_FDS.erase (client_ID);

			  ret = close (cl_fd);
			  DIE (ret < 0, "close");
			  FD_CLR (cl_fd, &server_FDS);

			  cout << "Client " << client_ID.
			    c_str () << " disconnected.\n";
			  goto loop_end;
			}
		      count = count + ret;
		    }

		  continue;
		}

	      if (!SF_number)
		{
		  continue;
		}

	      save_SFS[client_ID].push_back (message);
	    }

	  FD_CLR (SV_FD_UDP, &server_FDS_copy);
	}

      if (FD_ISSET (SV_FD_TCP, &server_FDS_copy))
	{
	  struct sockaddr_in cl_addr_stream;
	  socklen_t cl_addr_stream_len = sizeof (cl_addr_stream);

	  int client_FD_TCP = accept (SV_FD_TCP,
				      (struct sockaddr *) &cl_addr_stream,
				      &cl_addr_stream_len);
	  DIE (client_FD_TCP < 0, "accept");

	  ret =
	    setsockopt (client_FD_TCP, IPPROTO_TCP, TCP_NODELAY, &(ret = 1),
			sizeof (ret));
	  DIE (ret < 0, "setsockopt");

	  FD_SET (client_FD_TCP, &server_FDS);
	  server_FDS_max = max (server_FDS_max, client_FD_TCP);

	  FD_CLR (SV_FD_TCP, &server_FDS_copy);
	}

      if (FD_ISSET (STDIN_FILENO, &server_FDS_copy))
	{
		/* Se analizeaza comenzile */
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

	  if (command_buffer != NULL)
	    {
			/* Daca se doreste exit */
	      if (strcmp (command_buffer, "exit") != 0)
		{
		  fprintf (stderr, "No command.\n");
		  continue;
		}
	      else
		break;
	    }
	  else
	    {
	      fprintf (stderr, "No command.\n");
	      continue;
	    }

	  fprintf (stderr, "Wrong command %s.\n", command_buffer);

	  FD_CLR (STDIN_FILENO, &server_FDS_copy);
	}

      for (int client_FD = 0; client_FD <= server_FDS_max; ++client_FD)
	{
	  if (!FD_ISSET (client_FD, &server_FDS_copy))
	    {
	      continue;
	    }

	  length = sizeof (message.message_tcp.header);
	  count = 0;

	  for (bool header = false; count < length;)
	    {
	      ret =
		recv (client_FD, &message.buffer[count], length - count, 0);
	      if (ret <= 0)
		{
		  string & cl_id = clients_IDS[client_FD];

		  clients_IDS.erase (client_FD);
		  clients_FDS.erase (cl_id);

		  if (ret == 0)
		    {
		      ret = shutdown (client_FD, SHUT_RDWR);
		      DIE (ret < 0, "shutdown");
		    }

		  ret = close (client_FD);
		  DIE (ret < 0, "close");
		  FD_CLR (client_FD, &server_FDS);

		  printf ("Client %s disconnected.\n", cl_id.c_str ());

		  goto client_loop_end;
		}
	      count = count + ret;


	      if (count >= length && !header)
		{
		  length = length + ntohs (message.message_tcp.header.len);
		  header = true;
		}
	    }

	  message.message_tcp.header.len =
	    ntohs (message.message_tcp.header.len);
    /* Se analizeaza toate tipurile de mesaje TCP */
	  switch (message.message_tcp.header.type)
	    {
        /* Cazul in care se doreste unsubscribe */
	    case message_tcp::type::UNSUBSCRIBE:
	      {
		string & client_ID = clients_IDS[client_FD];

		count = 0;

		char *topic = &message.message_tcp.data[count];

		if (clients_topics[topic].count (client_ID) == 0)
		  {
			  /* Se trateaza erorile */
		    length = 0;
		    fprintf (stderr, "Client %s not subscribed to %s.\n",
			     client_ID.c_str (), topic);
		    strcpy (&message.message_tcp.data[length],
			    "Not subscribed to topic.\n");
		    length =
		      length + strlen (&message.message_tcp.data[length]) + 1;

		    message.message_tcp.header.len = htons (length);

		    length = length + sizeof (message.message_tcp.header);
		    count = 0;
           message.message_tcp.header.type =
		      message_tcp::type::ANSWER_SV;
		    for (; count < length;)
		      {
			ret =
			  send (client_FD, &message.buffer[count],
				length - count, 0);
			if (ret < 0)
			  {
			    clients_IDS.erase (client_FD);
			    clients_FDS.erase (client_ID);

			    ret = close (client_FD);
			    DIE (ret < 0, "close");
			    FD_CLR (client_FD, &server_FDS);
			    cout << "Client " << client_ID.
			      c_str () << " disconnected.\n";
			    goto client_loop_end;
			  }
			count = count + ret;
		      }
		    continue;
		  }

		clients_topics[topic].erase (client_ID);
		subscribers[client_ID].erase (topic);

		cout << "Client " << client_ID.
		  c_str () << " unsubscribed from " << topic << ".\n";

		length = 0;

		strcpy (&message.message_tcp.data[length],
			"Unsubscribed from topic.\n");
		length =
		  length + strlen (&message.message_tcp.data[length]) + 1;
		message.message_tcp.header.type =
		  message_tcp::type::ANSWER_SV;
		message.message_tcp.header.len = htons (length);
		length = length + sizeof (message.message_tcp.header);
		count = 0;
		for (; count < length;)
		  { 
		    ret =
		      send (client_FD, &message.buffer[count], length - count,
			    0);
		    if (ret < 0)
		      {
			clients_IDS.erase (client_FD);
			clients_FDS.erase (client_ID);

			ret = close (client_FD);
			DIE (ret < 0, "close");
			FD_CLR (client_FD, &server_FDS);

			cout << "Client " << client_ID.
			  c_str () << " disconnected.\n";
			goto client_loop_end;
		      }
		    count = count + ret;
		  }
		break;
	      }

	    case message_tcp::type::ID_CLIENT:
	      {
			  /* Cazul in care clientul trimite id-ul */
		char client_ID[CL_ID_LEN + 1] = { };
		strncpy (client_ID, message.message_tcp.data, CL_ID_LEN);
		struct sockaddr_in client_address;
		if (clients_FDS.count (client_ID))
		  {
			  /* Se trateaza erorile */
		    cout << "Client " << client_ID << " already connected.\n";
		    ret = shutdown (client_FD, SHUT_RD);
		    DIE (ret < 0, "shutdown");

		    length = 0;

		    strcpy (&message.message_tcp.data[length],
			    "Already connected.\n");
		    length =
		      length + strlen (&message.message_tcp.data[length]) + 1;

		    message.message_tcp.header.len = htons (length);

		    length = length + sizeof (message.message_tcp.header);
		    count = 0;
		    message.message_tcp.header.type =
		      message_tcp::type::ANSWER_SV;
		    for(;count < length;)
		      {
			ret =
			  send (client_FD, &message.buffer[count],
				length - count, 0);
			if (ret < 0)
			  {
			    clients_IDS.erase (client_FD);
			    clients_FDS.erase (client_ID);

			    ret = close (client_FD);
			    DIE (ret < 0, "close");
			    FD_CLR (client_FD, &server_FDS);

			    printf ("Client %s disconnected.\n", client_ID);
			    goto client_loop_end;
			  }
			count = count + ret;
		      }

		    ret = shutdown (client_FD, SHUT_WR);
		    DIE (ret < 0, "shutdown");
		    ret = close (client_FD);
		    DIE (ret < 0, "close");
		    FD_CLR (client_FD, &server_FDS);

		    continue;
		  }
		clients_FDS[client_ID] = client_FD;
		clients_IDS[client_FD] = client_ID;

		socklen_t cl_addr_len = sizeof (client_address);

		getpeername (client_FD, (struct sockaddr *) &client_address,
			     &cl_addr_len);
		cout << "New client " << client_ID << " connected from " <<
		  inet_ntoa (client_address.
			     sin_addr) << ":" << ntohs (client_address.
							sin_port) << ".\n";

		if (save_SFS.count (client_ID))
		  {
		  for (auto && message_iter:save_SFS[client_ID])
		      {
			length =
			  sizeof (message_iter.message_tcp.header) +
			  ntohs (message_iter.message_tcp.header.len);
			count = 0;

			 for (;count < length;)
			  {
			    ret =
			      send (client_FD, &message_iter.buffer[count],
				    length - count, 0);
			    if (ret < 0)
			      {
				clients_IDS.erase (client_FD);
				clients_FDS.erase (client_ID);

				ret = close (client_FD);
				DIE (ret < 0, "close");
				FD_CLR (client_FD, &server_FDS);

				printf ("Client %s disconnected.\n",
					client_ID);
				goto client_loop_end;
			      }
			    count = count + ret;
			  }
		      }
		  }
		else
		  continue;

		save_SFS.erase (client_ID);

		break;
	      }

	    case message_tcp::type::SUBSCRIBE:
	      {
			  /* Cazul in care se doreste subscribe */
		string & client_ID = clients_IDS[client_FD];
		count = 0;
		char *topic = &message.message_tcp.data[count];
		count = count + strlen (topic) + 1;

		if (clients_topics[topic].count (client_ID))
		  {
			  /* Se trateaza erorile */
		    length = 0;
		    fprintf (stdout, "Client %s already subscribed to %s.\n",
			     client_ID.c_str (), topic);
		    strcpy (&message.message_tcp.data[length],
			    "Already subscribed to topic.\n");
		    length =
		      length + strlen (&message.message_tcp.data[length]) + 1;

		    message.message_tcp.header.len = htons (length);

		    length = length + sizeof (message.message_tcp.header);
		    count = 0;
			message.message_tcp.header.type =
		      message_tcp::type::ANSWER_SV;
		    for (; count < length;)
		      {
			ret =
			  send (client_FD, &message.buffer[count],
				length - count, 0);
			if (ret < 0)
			  {
			    clients_FDS.erase (client_ID);
			    clients_IDS.erase (client_FD);
			    ret = close (client_FD);
			    DIE (ret < 0, "close");
			    FD_CLR (client_FD, &server_FDS);

			    printf ("Client %s disconnected.\n",
				    client_ID.c_str ());
			    goto client_loop_end;
			  }
			count = count + ret;
		      }
		    continue;
		  }

		char SF_number;
		memcpy (&SF_number, &message.message_tcp.data[count],
			sizeof (SF_number));
		subscribers[client_ID][topic] = SF_number;
		clients_topics[topic][client_ID] = SF_number;

		length = 0;
		strcpy (&message.message_tcp.data[length],
			"Subscribed to topic.\n");
		length =
		  length + strlen (&message.message_tcp.data[length]) + 1;

		message.message_tcp.header.len = htons (length);
		length = length + sizeof (message.message_tcp.header);
		message.message_tcp.header.type =
		  message_tcp::type::ANSWER_SV;
		count = 0;

		for (; count < length;)
		  {
		    ret =
		      send (client_FD, &message.buffer[count], length - count,
			    0);
		    if (ret < 0)
		      {
			clients_FDS.erase (client_ID);
			clients_IDS.erase (client_FD);

			ret = close (client_FD);
			DIE (ret < 0, "close");
			FD_CLR (client_FD, &server_FDS);

			printf ("Client %s disconnected.\n",
				client_ID.c_str ());
			goto client_loop_end;
		      }
		    count = count + ret;
		  }

		break;
	      }

	    default:
	      fprintf (stderr, "Strange message from client %s.\n",
		       clients_IDS[client_FD].c_str ());
	    }

	client_loop_end:
	  FD_CLR (client_FD, &server_FDS_copy);
	}
    loop_end:
      ;
    }

  FD_CLR (STDIN_FILENO, &server_FDS);

  ret = close (SV_FD_UDP);
  DIE (ret < 0, "close");
  FD_CLR (SV_FD_UDP, &server_FDS);

  ret = close (SV_FD_TCP);
  DIE (ret < 0, "close");
  FD_CLR (SV_FD_TCP, &server_FDS);
   /* Se dezactiveaza toti clientii */
  int client_FD = 0;
  for (; client_FD <= server_FDS_max; ++client_FD)
    {
      if (!FD_ISSET (client_FD, &server_FDS))
	{
	  continue;
	}

      ret = shutdown (client_FD, SHUT_RDWR);
      DIE (ret < 0, "shutdown");

      ret = close (client_FD);
      DIE (ret < 0, "close");
      FD_CLR (client_FD, &server_FDS);
    }

  return 0;
}
