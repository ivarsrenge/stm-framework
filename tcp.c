/*
 * tcp.c
 *
 *  Created on: Jul 19, 2025
 *      Author: User
 */




#ifdef USING_TCP
#include "tcp.h"
#include "console.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip.h"      // ← brings in “extern struct netif gnetif;”
#include "lwip/sockets.h"
#include "lwip/netdb.h"    // for gethostbyname()
#include "lwip/inet.h"     // for inet_addr() / inet_aton()
//#include <string.h>
#include <stdio.h>
#include "lwip/ip4_addr.h"   // for IP4_ADDR
#include "lwip/tcp.h"
 // http_client.c

 // bring in your netif to wait for link/up
 extern struct netif gnetif;

 // our TCP control block
 static struct tcp_pcb *hc_pcb;

 //================================================================
 // callbacks
 //================================================================
 /* error callback: called if the connection is aborted for any reason */
 static void
 hc_err(void *arg, err_t err)
 {
     LWIP_UNUSED_ARG(arg);
     printf("hc: connection aborted with err=%d\n", err);
     // cleanup
     hc_pcb = NULL;
 }

 /* recv callback: called whenever data arrives */
 static err_t hc_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
     if (err != ERR_OK || p == NULL) {
         // remote closed or error
         tcp_close(tpcb);
         hc_pcb = NULL;
         return ERR_OK;
     }

     // print payload to UART/console
     printf("TCP> %i bytes received\n", p->len);
     //fwrite(p->payload, 1, p->len, stdout);
     //fflush(stdout);

     // tell LwIP we have taken the data
     tcp_recved(tpcb, p->len);
     pbuf_free(p);
     return ERR_OK;
 }

 /* connected callback: called once the 3‑way handshake completes */
 static err_t hc_connected(void *arg, struct tcp_pcb *tpcb, err_t err)  {
     const char *req =
         "GET /test/index.htm HTTP/1.1\r\n"
         "Host: gdog.lv\r\n"
         "Connection: close\r\n"
         "\r\n";

     if (err != ERR_OK) {
         printf("hc: connect error %d\n", err);
         return err;
     }

     // send our GET request
     tcp_write(tpcb, req, strlen(req), TCP_WRITE_FLAG_COPY);
     tcp_output(tpcb);
     return ERR_OK;
 }

 //================================================================
 // public API
 //================================================================
 int tcpRequest(char* params) {
     ip4_addr_t server_ip;
     // gdog.lv  ≈ 93.175.33.25
     IP4_ADDR(&server_ip, 89,111,52,167);

     // 1) create new pcb
     hc_pcb = tcp_new();
     if (!hc_pcb) {
         printf("hc: tcp_new() failed\n");
         return;
     }

     // 2) set callbacks
     tcp_err(hc_pcb, hc_err);
     tcp_recv(hc_pcb, hc_recv);

     // 3) kick off connection
     err_t r = tcp_connect(hc_pcb, &server_ip, 80, hc_connected);
     if (r != ERR_OK) {
         printf("hc: tcp_connect failed %d\n", r);
         tcp_close(hc_pcb);
         hc_pcb = NULL;
     }
 }



	void tcpProcess(uint32_t param) {
		MX_LWIP_Process(); // test only
	}

	void tcpReqProc(uint32_t par) {
		tcpRequest(NULL);
	}

	int tcpLoop(char* args) {
		/*
	   dhcp_release(&gnetif);
	  dhcp_stop(&gnetif);

	  IP4_ADDR(&gnetif.ip_addr, 192,168,1,50);
	  IP4_ADDR(&gnetif.netmask, 255,255,255,0);
	  IP4_ADDR(&gnetif.gw, 192,168,1,1);

	  netif_set_up(&gnetif);
	  netif_set_link_up(&gnetif);
	  */
		if (taskExists(&tcpReqProc)) {
			taskRemove(&tcpReqProc);
		} else repeat("TCP_REQ", ST_SEC * 2, &tcpReqProc);
	}

	int tcpCheck(char* args) {
		if (netif_is_link_up(&gnetif)) {
			printf("[LWIP] LINK UP, IP = %s\n",
			   ip4addr_ntoa(&gnetif.ip_addr));
		} else {
			printf("[LWIP] LINK DOWN\n");
		}
	}

	void tcpInit() {
		consoleRegister("tcpcheck", &tcpCheck);
		consoleRegister("tcploop", &tcpLoop);
		consoleRegister("tcprequest", &tcpRequest);

		while (gnetif.ip_addr.addr == 0)	{
			MX_LWIP_Process();  // handles input + sys_check_timeouts()
		}

		/* Now we have an IP: print it */
		char ip[20];
		strcpy(ip, ip4addr_ntoa(&gnetif.ip_addr));
		printf("Network set - IP: %s\n", ip);

		tTask* proc = repeat("TCP_PR", ST_MS, &tcpProcess);
		proc->timeout = 1000 * ST_SS * 30;
		proc->realtime_fail = ST_SEC;

		tcpRequest(NULL);
		/*tcpStats(0);





	  	  repeat("TCP_CN", ST_SEC * 5, &http_client_start);

	  	http_client_start(0);*/


	}




#endif


