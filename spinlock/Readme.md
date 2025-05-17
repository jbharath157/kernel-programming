# Spinlocks

The most common lock in the Linux kernel is the spin lock.

Spinlocks are used to protect short code sections that comprise just a few C statements and are therefore quickly executed and exited.

A spin lock is a lock that can be held by at most one thread of execution.

---

## When the thread tries to acquire lock which is already held?

The thread busy loops/spins waiting for the lock to become available.

## When the thread tries to acquire lock which is available?

The thread acquires the lock and continues.

---

# Spinlock Methods

## Header File:
```c
#include <linux/spinlock.h>
```

## Data Structure:
```c
spinlock_t
```

## Methods

```c
DEFINE_SPINLOCK(my_lock);  // Equivalent to:
                           // spinlock_t my_lock = __SPIN_LOCK_UNLOCKED(my_lock);
```

From `<linux/spinlock_types.h>`:
```c
#define DEFINE_SPINLOCK(x) spinlock_t x = __SPIN_LOCK_UNLOCKED(x)
```

### 🔒 To lock a spin lock:
```c
void spin_lock(spinlock_t *lock);
```

### 🔓 To unlock a spin lock:
```c
void spin_unlock(spinlock_t *lock);
```


# Static way

```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");

DEFINE_SPINLOCK(my_lock);

static int __init test_hello_init(void)
{
    spin_lock(&my_lock);
    pr_info("Starting critical region\n");
    pr_info("Ending critical region\n");
    spin_unlock(&my_lock);
    return -1;
}

static void __exit test_hello_exit(void)
{
}

module_init(test_hello_init);
module_exit(test_hello_exit);
```
---

# Dynamic way

```
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
spinlock_t *my_lock;

static int __init test_hello_init(void)
{
    my_lock = kmalloc(sizeof(spinlock_t), GFP_KERNEL);
    spin_lock_init(my_lock);
    spin_lock(my_lock);
    pr_info("Starting critical region\n");
    pr_info("Ending critical region\n");
    spin_unlock(my_lock);
    kfree(my_lock);
    return -1;
}

static void __exit test_hello_exit(void)
{
}

module_init(test_hello_init);
module_exit(`test_hello_exit);
```

---


# Will spin lock exist on uniprocessor machines?

## When CONFIG_PREEMPT is not set / kernel preemption disabled

Spinlocks are defined as **empty operations** because critical sections cannot be entered by several CPUs at the same time on a uniprocessor system.

## When CONFIG_PREEMPT is set

In order to protect critical sections from **kernel preemption** (even on uniprocessor), spinlocks are redefined as:

```c
spin_lock   = preempt_disable();
spin_unlock = preempt_enable();
```

This ensures that even on a **uniprocessor**, the code within the spinlock is not preempted by another kernel thread.


## Note:

What happens if i acquire a lock which is already held by the same CPU?

**Spin locks are not recursive**

Unlike spin lock implementations in other operating systems and threading libraries, the Linux kernel’s spin locks are not recursive

This means that if you attempt to acquire a lock you already hold, you will spin, waiting for yourself to release the lock.

But because you are busy spinning, you will never release the lock and you will deadlock.

