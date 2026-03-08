=========================
DMBox Auxv Usage Guide
=========================

Overview
========

When a program is started with the ``-dmbox`` command-line option, the kernel
enables DMBox for this process and writes an auxiliary vector entry:

- ``AT_DMBOX_ENABLED = 1``

User-space runtime or loader code can read this auxv value to decide whether
DMBox sandbox behavior should be enabled for untrusted ``.so`` files.

Auxv Key
========

The key is defined in ``include/uapi/linux/auxvec.h``:

- ``AT_DMBOX_ENABLED`` (value ``60``)

Recommended Method: getauxval()
===============================

Use ``getauxval(AT_DMBOX_ENABLED)`` from ``<sys/auxv.h>``.

.. code-block:: c

   #include <sys/auxv.h>
   #include <linux/auxvec.h>
   #include <stdio.h>

   int main(void)
   {
       unsigned long enabled = getauxval(AT_DMBOX_ENABLED);

       if (enabled == 1) {
           printf("DMBox is enabled\n");
       } else {
           printf("DMBox is disabled\n");
       }
       return 0;
   }

Fallback Method: /proc/self/auxv
================================

If ``getauxval()`` is unavailable, parse ``/proc/self/auxv`` entries as
``(a_type, a_val)`` pairs and look for ``a_type == AT_DMBOX_ENABLED`` and
``a_val == 1``.

Behavior Contract
=================

- If process is started with ``-dmbox``: auxv contains ``AT_DMBOX_ENABLED=1``.
- Without ``-dmbox``: process runs as a normal program, and this key is not set
  to ``1``.
