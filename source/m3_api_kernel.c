//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#define _POSIX_C_SOURCE 200809L

#include "m3_api_kernel.h"

#include "m3_exception.h"

// #include <linux/printk.h>
// #include <linux/string.h>
#include <string.h>

#ifdef __aarch64__
    #undef and
    #undef or
#endif

// #include <linux/ktime.h>
#include <sys/callout.h>

#ifdef __aarch64__
    #define and &&
    #define or ||
#endif

// #include <linux/timekeeping.h>

// #include <linux/netfilter.h>
// #include <linux/skbuff.h>
// #include <linux/in.h>
// #include <linux/ip.h>
// #include <linux/netdevice.h>
// #include <linux/if_arp.h>
// #include <linux/if_ether.h>
// #include <linux/if_packet.h>
// #include <linux/inet.h>
// #include <net/ip.h>
// #include <net/udp.h>
// #include <linux/netfilter_ipv4.h>
// #include <linux/delay.h>
// #include <net/sock.h>
// #include <linux/udp.h>
// #include <linux/socket.h>
// #include <linux/net.h>
// #include <linux/spinlock.h>

#define IS_WAIT 0

typedef uint32_t wasm_ptr_t;
//typedef uint32_t wasm_size_t;

static u64 start_time;


//TODO: expose test - clean this part
//      no related to ping test 
m3ApiRawFunction(m3_libc_test)
{
    m3ApiReturnType (int);
    
    m3ApiGetArg(int32_t, a);
    m3ApiGetArg(int32_t, b);
    
    int r = a+b;
    
    m3ApiReturn(r);
}

m3ApiRawFunction(m3_libc_test1)
{
    m3ApiReturnType (uint32_t);
    
    m3ApiGetArg(int32_t, a);
    m3ApiGetArg(int32_t, b);
    
    int32_t r = a-b;
    
    m3ApiReturn(r);
}

int testA = 5;
m3ApiRawFunction(m3_kernel_get_addr)
{
    m3ApiReturnType (void**);
    
    m3ApiGetArgMem  (void**,     addr);

    *addr = (void*)&testA;

    pr_info("Add A: %p\n", &testA);
    pr_info("Add a: %p\n", *addr);

    pr_info("Val A: %d\n", *(&testA));
    pr_info("Val a: %d\n", *((int*) (*addr)));

    m3ApiReturn(addr);
}

m3ApiRawFunction(m3_kernel_resolve_addr)
{
    m3ApiReturnType (void**);
    
    m3ApiGetArgMem  (void**,     addr);

    pr_info("Resolve add: %p\n", *addr);

    if(*addr == &testA)
    {
        pr_info("Yay\n");
        pr_info("Resolved Val: %d\n", *((int*)*addr));
    }
    else
        pr_info("Nah\n");

    m3ApiReturn(0);
}


// Utility APIs

// Derive CPU cycle from Time Stamp Counter
//  Windows
#ifdef _WIN32

#include <intrin.h>
uint64_t my_rdtsc(){
    return __rdtsc();
}

#elif __aarch64__
unsigned my_rdtsc (void)
{
  uint64_t cc;
  asm volatile("mrs %0, cntvct_el0" : "=r" (cc));
  return cc;
}

//  Linux/GCC
#else

uint64_t my_rdtsc(void){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

#endif

m3ApiRawFunction(m3_kernel_pr_str)
{
    m3ApiReturnType (uint32_t);

    m3ApiGetArgMem  (char*,       i_str);

    size_t len = strlen(i_str) + 1; // for '\0'
    m3ApiCheckMem(i_str, strlen(i_str)+1);

    pr_info("%s",i_str);

    m3ApiReturn(len);
}

m3ApiRawFunction(m3_kernel_pr_int)
{
    m3ApiReturnType(uint32_t);
    m3ApiGetArg(int, i);

    pr_info("%d\n", i);

    m3ApiReturn(i);
}

m3ApiRawFunction(m3_kernel_pr_info)
{
    m3ApiReturnType (uint32_t);

    m3ApiGetArgMem  (char*,       i_str);
    m3ApiGetArg     (int,         arg);

    size_t len = strlen(i_str) + 1;
    m3ApiCheckMem(i_str, len);

    pr_info("%s: %d",i_str, arg);

    m3ApiReturn(len);
}

m3ApiRawFunction(m3_kernel_pr_ptr)
{
    m3ApiGetArgMem  (void*,       ptr);

    m3ApiCheckMem(ptr, sizeof(char));
    
    pr_info("pr_ptr: %p", ptr);
    
    m3ApiSuccess();
}

m3ApiRawFunction(m3_kernel_time)
{
    m3ApiReturnType(uint64_t);

    uint64_t t = ktime_get_raw_ns()/1000; //us

    m3ApiReturn(t);
}

m3ApiRawFunction(m3_kernel_tick)
{
    m3ApiReturnType(uint64_t);

    uint64_t t = my_rdtsc();

    m3ApiReturn(t);
}


//Caller for native kernel socket
// m3ApiRawFunction(m3_kernel_ping)
// {
//     m3ApiReturnType (uint32_t);
//     int32_t r = send_rtt_init();
    
//     m3ApiReturn(r);
// }

// m3ApiRawFunction(m3_kernel_pong)
// {
//     m3ApiReturnType (uint32_t);
//     int32_t r = pong_rtt_init();

//     m3ApiReturn(r);
// }

// m3ApiRawFunction(m3_kernel_close_receive)
// {
//     m3ApiReturnType (uint32_t);
//     pong_rtt_exit();

//     m3ApiReturn(0);
// }

//Network Globals
// static struct sockaddr_in recvaddr;
// static struct socket *sock;
// static char* my_ip;
// static char* recv_ip;
// static short int my_port;
// static short int recv_port;

// static struct nf_hook_ops nfho;
// static int counter = 0;
// static int packbuff = 0;

#if IS_WAIT == 1
static wait_queue_head_t wq;
#endif

// Spin lock -- protect the variables that being accessed by multipled thread
// Disable now since it seems to not affect the data integrity
//static spinlock_t sl;
//static unsigned long sl_flags; 


//Network APIs
// m3ApiRawFunction(m3_kernel_net_config)
// {
//     pr_info("Configuring network settings...\n");
//     m3ApiReturnType(uint32_t);

//     m3ApiGetArgMem  (char*,       i_ip);
//     m3ApiGetArg     (short int,   i_p);
//     m3ApiGetArgMem  (char*,       r_ip);
//     m3ApiGetArg     (short int,   r_p);

//     my_ip = kmalloc(strlen(i_ip), GFP_KERNEL);
//     strcpy(my_ip, i_ip);
//     my_port = i_p;

//     recv_ip = kmalloc(strlen(r_ip), GFP_KERNEL);
//     strcpy(recv_ip, r_ip);
//     recv_port = r_p;

//     pr_info("My ip: %s, my port: %d\n", my_ip, my_port);
//     pr_info("Recv ip: %s, recv port: %d\n", recv_ip, recv_port);

//     m3ApiReturn(1);
// }

// unsigned int hook_func_ping(void *priv, struct sk_buff *skb,
// 				const struct nf_hook_state *state) {
// 	if(skb) {
// 		struct udphdr *udph = NULL;
// 		struct iphdr *iph = NULL;
// 		u8 *payload;    // The pointer for the tcp payload.

// 		iph = ip_hdr(skb);
// 		if(iph) {
// 		    //printk("IP:[%pI4]-->[%pI4];\n", &iph->saddr, &iph->daddr);
// 			__be32 receiverAddr = in_aton(recv_ip);
// 			__be32 sourceAddr = iph->saddr;

// 			if (receiverAddr == sourceAddr) {
// 				//printk("IP:[%pI4]-->[%pI4];\n", &iph->saddr, &iph->daddr);
// 				switch (iph->protocol) {
// 					case IPPROTO_UDP:
// 						/*get the udp information*/
// 						udph = (struct udphdr *)(skb->data + iph->ihl*4);
// 						payload = (char *)udph + (char)sizeof(struct udphdr);

//                         __be16 p = udph->dest;
//                         //pr_info("Dest port: %u\n", htons(udph->dest));
//                         //pr_info("My port: %u\n", my_port);
//                         if(my_port != htons(udph->dest))
//                         {
//                             //pr_info("Not for me\n");
//                             break;
//                         }

//                         //spin_lock_irqsave(&sl, sl_flags); 
// 						packbuff = 1;
// 						counter++;
//                         //spin_unlock_irqrestore(&sl, sl_flags);

// #if IS_WAIT == 1
//                         //Time stamp before wake up receive
//                         start_time = ktime_get_raw_ns();
//                         wake_up(&wq); 
// #endif

// 						break;
// 					default:
// 						pr_err("unknown protocol!\n");
// 						break;
// 				}
// 			}
// 		} else {
// 				pr_err("iph is null\n");
// 		}
// 	} else {
// 			pr_err("skb is null\n");
// 	}


// 	return NF_ACCEPT;
// }

// m3ApiRawFunction(m3_kernel_create_socket)
// {
//     //Assume only one socket and return socket index
//     //The index returned is just for a placeholder for now
    
//     pr_info("Creating socket..\n");
//     m3ApiReturnType(uint32_t);
    
//     memset(&recvaddr,0,sizeof(recvaddr));
// 	recvaddr.sin_family = AF_INET;
// 	recvaddr.sin_port = htons(my_port);
// 	recvaddr.sin_addr.s_addr = in_aton(my_ip);

// 	if(sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock) < 0){
// 		printk(KERN_ALERT "sock_create_kern error\n");
// 		goto error;
// 	}
// 	int err;
// 	if ((err = kernel_bind (sock, (struct sockaddr*)&recvaddr, sizeof(struct sockaddr) )) < 0) {
// 		printk(KERN_ALERT "sock bind error: %d\n", err);
// 		goto error;
// 	}

//     pr_info("Sock created: %p\n", sock);

//     // Resigter the hook function
//     nfho.hook = hook_func_ping;
// 	nfho.hooknum  = NF_INET_PRE_ROUTING;
// 	nfho.pf = AF_INET;
// 	nfho.priority = NF_IP_PRI_FIRST;
// 	nf_register_net_hook(&init_net, &nfho);
  
// #if IS_WAIT == 1
//     // Initialize wait queue 
//     init_waitqueue_head(&wq);
// #endif    

//     // Initialize spin lock
//     //spin_lock_init(&sl); 
    
//     m3ApiReturn(0);//Assume only one socket; return index 0
// error:
// 	sock = NULL;
//     m3ApiReturn(-1);

// }

// m3ApiRawFunction(m3_kernel_release_socket)
// {
//     m3ApiReturnType(uint32_t);
    
//     m3ApiGetArgMem  (int, idx);

//     pr_info("Releasing socket\n");
    
//     nf_unregister_net_hook(&init_net, &nfho);
// 	sock_release(sock);
//     kfree(my_ip);
//     kfree(recv_ip);
//     //kfree(sock);//Only make sense for only one socket
//     m3ApiReturn(1);
// }

/* 
//Placeholder for executing close alone
//If release unsuccessfully,
//execute this via wasm3
m3ApiRawFunction(m3_kernel_close_socket)
{
    //Used to closed dangling socket if necessary 
    m3ApiReturnType(uint32_t);

    nf_unregister_net_hook(&init_net, &nfho);
    sock_release(sock);
    kfree(my_ip);
    kfree(recv_ip);

    pr_info("Socket released\n");

    m3ApiReturn(1);
}
*/

// m3ApiRawFunction(m3_kernel_send)
// {
//     m3ApiReturnType(uint32_t);

//     m3ApiGetArgMem  (int,         idx);
//     m3ApiGetArgMem  (char*,       buff);
//     m3ApiGetArg     (size_t,      length);

//     struct msghdr        msg;
// 	struct kvec        iov = {0};
// 	int                len;

// 	// Construct target address
//     struct sockaddr_in target_addr;
// 	memset(&target_addr,0,sizeof(target_addr));
// 	target_addr.sin_family = AF_INET;
// 	target_addr.sin_port = htons(recv_port);
// 	target_addr.sin_addr.s_addr = in_aton(recv_ip);
 
// 	memset(&msg,0,sizeof(msg));
// 	msg.msg_flags = MSG_DONTWAIT|MSG_NOSIGNAL;       
// 	msg.msg_name = (struct sockaddr *)&target_addr;
// 	msg.msg_namelen = sizeof(target_addr);

// 	iov.iov_base     = (void *)buff;
// 	iov.iov_len      = length;

// 	len = kernel_sendmsg(sock, &msg, &iov, 1, length);
    
//     m3ApiReturn(len);
// }

// m3ApiRawFunction(m3_kernel_receive)
// {
//     m3ApiReturnType(int);
  
// #if IS_WAIT == 1
//     //Put receive into wait queue until receiving a packet
//     int isTimeout = wait_event_timeout(wq, packbuff==1, 200);
//     //pr_info("Elapsed time:%lld\n", ktime_get_raw_ns()-start_time);//Timestamp the wake up time
// #else
//     int isTimeout = 0; 
//     ktime_t start = ktime_get();
//     while ( ktime_ms_delta(ktime_get(), start) < 10000 ) {
// 			if (packbuff == 1) { isTimeout = 1; break; }
// 		}
// #endif
    
//     if(isTimeout == 0)
//     {
//         pr_info("Timeout: failed to received a packet...\n");
//         m3ApiReturn(-1);
//     }else
//     {
//         //spin_lock_irqsave(&sl, sl_flags); 
//         packbuff = 0;
//         //spin_unlock_irqrestore(&sl, sl_flags);
//         m3ApiReturn(counter);
//     }
// }

// m3ApiRawFunction(m3_kernel_accessMem)
// {
//     m3ApiReturnType(int);
//     m3ApiGetArg  (int,         s);

//     int *mem = kmalloc(s, GFP_KERNEL);
//     for(int i=0; i<s; i++)
//     {
//         *(mem+i) = i;
//     }
//     kfree(mem);

//     m3ApiReturn(0);
// }

// m3ApiRawFunction(m3_kernel_accessComp)
// {
//     m3ApiReturnType(int);
//     m3ApiGetArg  (int,         m);

//     int i = 5;
//     for(int j=0;j<m;j++)
//     {
//        int c = j*j;
//        int d = c*j+1;
//        int e = d*j*2;
//     }

//     m3ApiReturn(0);
// }


static
M3Result  SuppressLookupFailure (M3Result i_result)
{
    if (i_result == m3Err_functionLookupFailed)
        return m3Err_none;
    else
        return i_result;
}



M3Result  m3_LinkKernel  (IM3Module module)
{
    M3Result result = m3Err_none;

    const char* env = "env";

// Test functions
_   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "test",            "i(ii)",   &m3_libc_test)));
_   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "get_addr",        "*(*)",    &m3_kernel_get_addr)));
_   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "resolve_addr",    "i(*)",    &m3_kernel_resolve_addr)));

// Utility
_   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "pr_str",         "i(*)",     &m3_kernel_pr_str)));
_   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "pr_int",         "i(i)",      &m3_kernel_pr_int)));
_   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "pr_info",        "i(*i)",    &m3_kernel_pr_info)));
_   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "pr_ptr",         "(*)",      &m3_kernel_pr_ptr)));
_   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "time",           "i()",      &m3_kernel_time)));
_   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "tick",           "i()",      &m3_kernel_tick)));

// Caller for native kernel socket
// _   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "wasm_send",       "i()",     &m3_kernel_ping)));
// _   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "wasm_receive",       "i()",     &m3_kernel_pong)));
// _   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "close_receive",   "i()",     &m3_kernel_close_receive)));


// Network 
// _   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "net_config",     "i(*i*i)",  &m3_kernel_net_config)));
// _   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "create_socket",  "i()",     &m3_kernel_create_socket)));
// _   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "release_socket", "i(i)",     &m3_kernel_release_socket)));
//_   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "close_socket",   "i()",      &m3_kernel_close_socket)));
// _   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "send",           "i(i*i)",   &m3_kernel_send)));
// _   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "receive",        "i()",      &m3_kernel_receive)));

// Benchmark tool -- adding computation or memory accessing to the process
// Compute is used in the experiments
// Mem does not fit into the story due to the lack of physical memory; 
// using external tool like stress-ng to create processes to generate memory bomb 
// _   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "accessMem",        "i(i)",      &m3_kernel_accessMem)));
// _   (SuppressLookupFailure (m3_LinkRawFunction (module, env, "accessComp",        "i(i)",      &m3_kernel_accessComp)));


_catch:
    return result;
}

