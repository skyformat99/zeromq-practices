/**
 * @file czmq_fix.h
 *
 * @breif Fix the api differences between Windows and Linux, for example, there is
 * no 'zstr_sendf' in Linux, whereas 'zstr_send' from Linux behaves the same as
 * 'zstr_sendf' in Windows.
 */
#ifndef CZMQ_FIX_H_
#define CZMQ_FIX_H_

#ifndef WIN32
# define zstr_sendf zstr_send
#endif

#endif // CZMQ_FIX_H_
