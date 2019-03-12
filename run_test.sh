#!/bin/bash


PAGE_LIST=`seq 0 9`
COPY_METHOD="seq"
EXCHANGE_METHOD="u64 per_page"
MULTI="1"


for METHOD in ${EXCHANGE_METHOD}; do

    RES_FOLDER="stats_2mb_${METHOD}_exchange"
    if [ ! -d ${RES_FOLDER} ]; then
        mkdir ${RES_FOLDER}
    fi

    for I in `seq 1 5`; do
        for MT in ${MULTI}; do
            for COPY in ${COPY_METHOD}; do
                for N in ${PAGE_LIST}; do
                    NUM_PAGES=$((1<<N))

                    echo "NUM_PAGES: "${NUM_PAGES}", COPY: "${COPY}", MT: "${MT}

                    if [[ "x${I}" == "x1" ]]; then
                        numactl -N 0 ./thp_move_pages ${NUM_PAGES} 2>/dev/null | grep -A 3 "\(Total_cycles\|Test successful\)" > ./${RES_FOLDER}/${COPY}_${MT}_page_order_${N}_exchange_no_batch
                    else
                        numactl -N 0 ./thp_move_pages ${NUM_PAGES} 2>/dev/null | grep -A 3 "\(Total_cycles\|Test successful\)" >> ./${RES_FOLDER}/${COPY}_${MT}_page_order_${N}_exchange_no_batch
                    fi

                    sleep 5
                done
            done
        done
    done
done
