#ifndef CONTY_CAPABILITY_H
#define CONTY_CAPABILITY_H

/*
 * Permitted - Capabilities that a process can propagate to its children
 *             and can itself use
 * Inheritable - Capabilities preserved across an execve boundary, i.e
 *               after an exec, the inheritable capabilities of the parent
 *               remain inheritable in the child. Inheritable capabilities
 *               are added to the permitted set when executing a program
 *               that has appropriate bits set in the file inheritable set.
 *
 *               Not preserved across an execve boundary if not root
 *
 * Effective - The set of capabilities used by the kernel to perform permission checks
 * Bounding - Used to limit the capabilities gained during execve
 * Ambient - Set of capabilities preserved across execve when not root.
 *           No capability can ever be ambient if it is not both permitted
 *           and inheritable
 *           Can be directly modified using prctrl
 *
 *           Executing a program that changes the effective uid or gid
 *           due to set-user-id and set-group-id bits will clear the ambient set
 *           Ambient caps are added to the permitted set and assigned
 *           to the effective set when execve is called.
 *
 * Children created via fork inherit copies of their parent's capability sets
 */
#include <sys/capability.h>

#endif //CONTY_CAPABILITY_H
