#include "sandbox.h"
#include <string.h>

#include <sched.h>
int main(int argc, char *argv[])
{
    struct conty_sandbox sb;
    memset(&sb, 0, sizeof(struct conty_sandbox));

    sb.ns.new_clone_flags = (CLONE_NEWUSER | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWNET | CLONE_NEWUTS);
    conty_sandbox_create(&sb);

}