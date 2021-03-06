/*

Copyright (c) 2014, Martin Åberg All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. The names of the copyright holder(s) may not be used to endorse or promote
products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <martos/martos.h>
#include "private.h"

void msgport_init(MsgPort *const port)
{
    SignalNumber signum;

    signum = signal_allocate(-1);
    assert(-1 != signum);
    port->signal = 1 << signum;
    port->action = MSGPORT_SIGNAL;
    port->task = running;
    list_init((&port->message_list));
}

Message *msgport_wait(MsgPort *const port)
{
    /* TODO: Check consistency of port->signal. */
    Message *msg;

    disable();
    while (list_is_empty(&port->message_list)) {
        signal_wait(port->signal);
    }
    /* List is not empty so we can peek it. */
    msg = (Message *) port->message_list.head.next;
    enable();
    return msg;
}

Message *msgport_get(MsgPort *const port)
{
    Message *msg;

    disable();
    msg = (Message *) list_rem_head(&port->message_list);
    enable();
    return msg;
}

void msgport_send(MsgPort *const port, Message *const message)
{
    /* TODO: Check consistency of port->signal. */
    /* Assume that the port does not disappear, for example
    here. */

    /* Some rearrangements can be done to decrease time in
    the disable() state. */
    disable();
    assert(
        MSGPORT_SIGNAL == port->action ||
        MSGPORT_IGNORE == port->action
    );
    list_add_tail(&port->message_list, (Node *) message);
    /* Assume only port owner modifies non-list fields. */
    if ((NULL != port->task) &&
      (MSGPORT_SIGNAL == port->action)) {
        /* A task exists that waits for messages on this
        port. */
        signal_send(port->task, port->signal);
    } else {
        /* No task is waiting on the port. Just leave the
        message. */
    }
    enable();
}

void msgport_reply(Message *const message)
{
    /* Assume that the port does not disappear, for example
    here. Furthermore, we own the message at this time so we
    may access it freely. */
    MsgPort *port;
    port = message->reply_port;
    assert(NULL != port);
    /* FIXME: Do some run-time error handling if port is
    NULL. */
    msgport_send(port, message);
}

