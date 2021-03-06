#/bin/bash

# Resnet50
# batch-size: 1
# SKX 8180 1S

source ./scripts/best_configs/common.sh $@
COMMON="$COMMON --f16c-opt=0"

# resnet50_res2a_branch2b
NSOCKETS=1 ./scripts/run.sh -c -i64 -h56 -o64 -H56 -n12 --tile-size=6 --execution-mode=0xa161 --blk-i=4 --blk-o=2 --flt-o=2 --flt-t=14 --pat-o=1 --output-as-blocked=true $COMMON --data-type-cfg=U8F32S8F32 --sampling-kind=2
# resnet50_res3a_branch2b
NSOCKETS=1 ./scripts/run.sh -c -i128 -h28 -o128 -H28 -n12 --tile-size=6 --execution-mode=0xa161 --blk-i=8 --blk-o=2 --flt-o=2 --flt-t=14 --pat-o=2 $COMMON --data-type-cfg=U8F32S8F32 --sampling-kind=2
# resnet50_res4a_branch2b
NSOCKETS=1 ./scripts/run.sh -c -i256 -h14 -o256 -H14 -n12 --tile-size=6 --execution-mode=0xa133 --blk-i=16 --blk-o=2 --flt-o=2 --flt-t=14 --pat-o=1 $COMMON --data-type-cfg=U8F32S8F32 --sampling-kind=2
# resnet50_res5a_branch2b
NSOCKETS=1 ./scripts/run.sh -c -i512 -h7 -o512 -H7 -n12 --tile-size=6 --execution-mode=0xa133 --blk-i=32 --blk-o=2 --flt-o=2 --flt-t=14 --pat-i=1 --pat-o=1 $COMMON --data-type-cfg=U8F32S8F32 --sampling-kind=2
