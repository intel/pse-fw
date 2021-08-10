.. _dma-sample:

Overview
********

This sample demonstrates how to use the DMA  API.

Building and Running
********************

The sample can be built and executed on boards supporting DMA.

Sample output
=============

.. code-block:: console
    testing DMA normal reload mode

    Preparing DMA Controller: Chan_ID=0, BURST_LEN=8
    Starting the transfer
    DMA transfer done
    potato
    DMA Passed

    Preparing DMA Controller: Chan_ID=0, BURST_LEN=8
    Starting the transfer
    reload block 1
    DMA transfer done
    likes
    potato
    DMA Passed

    Preparing DMA Controller: Chan_ID=0, BURST_LEN=8
    Starting the transfer
    reload block 1
    reload block 2
    DMA transfer done
    horse
    likes
    potato
    DMA Passed

    Preparing DMA Controller: Chan_ID=0, BURST_LEN=8
    Starting the transfer
    reload block 1
    reload block 2
    reload block 3
    DMA transfer done
    my
    horse
    likes
    potato
    DMA Passed

    testing DMA linked list mode

    Preparing DMA Controller: Chan_ID=0, BURST_LEN=8
    Starting the transfer
    DMA transfer done
    block count = 1
    potato
    DMA Passed

    Preparing DMA Controller: Chan_ID=0, BURST_LEN=8
    Starting the transfer
    DMA transfer done
    block count = 2
    likes
    potato
    DMA Passed

    Preparing DMA Controller: Chan_ID=0, BURST_LEN=8
    Starting the transfer
    DMA transfer done
    block count = 3
    horse
    likes
    potato
    DMA Passed

    Preparing DMA Controller: Chan_ID=0, BURST_LEN=8
    Starting the transfer
    DMA transfer done
    block count = 4
    my
    horse
    likes
    potato
    DMA Passed

.. note:: The values shown above might differ.
