/**
 * @file mdp.h
 *
 * @breif Majordomo Protocol content
 */
#ifndef MAJORDOMO_MDP_H_
#define MAJORDOMO_MDP_H_

#define MDPC_HEADER "MDPC01"
#define MDPW_HEADER "MDPW01"

#define MDPW_READY "\001"
#define MDPW_REQUEST "\002"
#define MDPW_REPLY "\003"
#define MDPW_HEARTBEAT "\004"
#define MDPW_DISCONNECT "\005"

static const char* mdps_commands[] = {
   "",
   "READY",   // 1
   "REQUEST",
   "REPLY",
   "HEARTBEAT",
   "DISCONNECT"
};

#endif // MAJORDOMO_MDP_H_
