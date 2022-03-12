#!/bin/bash

MAX_NUMA_NODE=2

#
# Test case randomly select page size from 4K/2M ~ Max
# Base page size 16G (1 << 22 * 4K)
# THP page size  4G ( 1 << 11 * 2M)
#
BASE_PAGE_MAX_ORDER=22
THP_PAGE_MAX_ORDER=11

dma_switch_count=0

prog_done=0

declare -A results
results[CPU-BASE]=0
results[CPU-THP]=0
results[DSA-BASE]=0
results[DSA-THP]=0

function dump_info()
{
	echo "move base pages success: DSA-BASE: ${results[DSA-BASE]}, DSA-THP: ${results[DSA-THP]} CPU-BASE: ${results[CPU-BASE]}, CPU-THP: ${results[CPU-THP]}"
	echo ""
}

trap 'onCtrlC' INT
function onCtrlC()
{
	echo "Ctrl + C pressed"
	dump_info
	prog_done=1
}


# Run test case until Ctrl + C
while [[ $prog_done == 0 ]]
do
	dma_switch_count=$((dma_switch_count+1))
	if [[ $((dma_switch_count%2)) == 0 ]]; then
		dma_switch_count=0
		## Switch page migration between CPU and DSA
		dma_enabled=$(($RANDOM%2))
		echo $dma_enabled > /sys/kernel/mm/migrate/dma_enabled
		sleep 1
        fi

	dma_enabled=`cat /sys/kernel/mm/migrate/dma_enabled`
	echo "Use DMA: $dma_enabled"

	order=$(($RANDOM%$BASE_PAGE_MAX_ORDER))

	# RANDOM two difference number for src/dst NUMA node
	src_node=$(($RANDOM%$MAX_NUMA_NODE))
	dst_node=$(($RANDOM%$MAX_NUMA_NODE))
	while [ $src_node == $dst_node ]
	do
		dst_node=$(($RANDOM%$MAX_NUMA_NODE))
	done

	# Test case for base page...
	echo "================ non_thp_move_pages ${src_node} ${dst_node} $((1<<$order))"
	./non_thp_move_pages ${src_node} ${dst_node} $((1<<$order))
	ret=$?
	if [[ $ret != 0 ]]; then
		echo "non_thp_move_pages ${src_node} ${dst_node} $((1<<$order)) error"
		dump_info
		exit
	fi

	if [[ "$dma_enabled" == "true" ]]; then
		results[DSA-BASE]=$((results[DSA-BASE]+1))
	else
		results[CPU-BASE]=$((results[CPU-BASE]+1))
	fi

	dump_info
	sleep 5

	# Test case for THP...
	thp_order=$(($RANDOM%$THP_PAGE_MAX_ORDER))
	echo "================ thp_move_pages ${src_node} ${dst_node} $((1<<$thp_order))"
	./thp_move_pages ${src_node} ${dst_node} $((1<<$thp_order))
	ret=$?
	if [[ $ret != 0 ]]; then
		echo "non_thp_move_pages ${src_node} ${dst_node} $((1<<$thp_order)) error"
		dump_info
		exit
	fi

	if [[ "$dma_enabled" == "true" ]]; then
		results[DSA-THP]=$((results[DSA-THP]+1))
	else
		results[CPU-THP]=$((results[CPU-THP]+1))
	fi

	dump_info
	sleep 5

done

dump_info
