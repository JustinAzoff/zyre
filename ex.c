//  The Ironhouse Pattern
//
//  This is exactly the same example but broken into two threads
//  so you can better see what client and server do, separately.

#include <zyre.h>
#include <czmq.h>

int main (void)
{
    //  Create the certificate store directory and client certs

    zactor_t *auth = zactor_new(zauth, NULL);
    assert (auth);
    zstr_sendx (auth, "VERBOSE", NULL);
    zsock_wait (auth);
    zstr_sendx (auth, "CURVE", CURVE_ALLOW_ANY, NULL);
    zsock_wait (auth);

    zcert_t *node1_cert = zcert_new ();
    zcert_t *node2_cert = zcert_new ();

    assert (node1_cert);
    assert (node2_cert);

    zyre_t *node1 = zyre_new ("node1");
    zyre_t *node2 = zyre_new ("node2");

    assert (node1);
    assert (node2);

    assert (streq (zyre_name (node1), "node1"));
    zyre_set_header (node1, "X-HELLO", "World");

    zyre_set_verbose (node1);
    zyre_set_verbose (node2);

    zyre_set_curve_key_public(node1, zcert_public_txt (node1_cert));
    zyre_set_curve_key_secret(node1, zcert_secret_txt (node1_cert));

    zyre_set_curve_key_public(node2, zcert_public_txt (node2_cert));
    zyre_set_curve_key_secret(node2, zcert_secret_txt (node2_cert));

    const char *gossip_cert;
    gossip_cert = zcert_public_txt (node1_cert);

    zyre_gossip_bind(node1, "tcp://*:9001");
    zyre_gossip_connect(node2, "tcp://127.0.0.1:9001|%s", gossip_cert);

    zyre_start(node1);
    zsock_wait(node1);
    zyre_start(node2);
    zsock_wait(node2);

    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");

    // Give them time to join their groups

    zclock_sleep (250);
    zyre_dump (node1);

    zyre_shouts (node1, "GLOBAL", "Hello, World");

    //  Second node should receive ENTER, JOIN, and SHOUT
    zmsg_t *msg = zyre_recv (node2);
    assert (msg);
    char *command = zmsg_popstr (msg);
    assert (streq (command, "ENTER"));
    zstr_free (&command);
    assert (zmsg_size (msg) == 4);
    char *peerid = zmsg_popstr (msg);
    char *name = zmsg_popstr (msg);
    assert (streq (name, "node1"));
    zstr_free (&name);
    zframe_t *headers_packed = zmsg_pop (msg);

    zyre_stop (node1);
    zyre_stop (node2);

    zyre_destroy (&node1);
    zyre_destroy (&node2);
    zactor_destroy (&auth);


    return 0;
}
