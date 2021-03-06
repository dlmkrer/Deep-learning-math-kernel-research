#/bin/bash

# Resnet50
# batch-size: 1
# SKX 8180 1S

source ./scripts/best_configs/common.sh $@

# resnet50_res2a_branch2b 6.75T
NSOCKETS=1 ./scripts/run.sh -c -i64 -h56 -o64 -H56 -n1 --tile-size=6 --execution-mode=0xa161 --blk-i=4 --blk-o=2 --flt-o=2 --flt-t=7 --pat-o=1 --output-as-blocked=true $COMMON --data-type-cfg=U8F32S8F32 --sampling-kind=2
# resnet50_res3a_branch2b 5.62T
NSOCKETS=1 ./scripts/run.sh -c -i128 -h28 -o128 -H28 -n1 --tile-size=6 --execution-mode=0xa133 --blk-i=8 --blk-o=2 --flt-o=2 --flt-t=13 --pat-o=1 $COMMON --streaming-output=2 $COMMON --data-type-cfg=U8F32S8F32 --sampling-kind=2
# resnet50_res4a_branch2b 4.95T
NSOCKETS=1 ./scripts/run.sh -c -i256 -h14 -o256 -H14 -n1 --tile-size=4 --execution-mode=0xa133 --blk-i=16 --blk-o=2 --flt-o=2 --flt-t=10 $COMMON --data-type-cfg=U8F32S8F32 --sampling-kind=2
# resnet50_res5a_branch2b 3.19T
NSOCKETS=1 ./scripts/run.sh -c -i512 -h7 -o512 -H7 -n1 --tile-size=4 --execution-mode=0xa133 --blk-i=32 --blk-o=1 --flt-o=2 --flt-t=8 $COMMON --data-type-cfg=U8F32S8F32 --sampling-kind=2


# a161 Ir case modified from resnet50_res2a_branch2b
NSOCKETS=1 ./scripts/run.sh -c -i66 -h56 -o64 -H56 -n1 --tile-size=6 --execution-mode=0xa161 --blk-i=5 --blk-o=1 --flt-o=2 --flt-t=7 --pat-o=1 --output-as-blocked=true $COMMON --input-format=nchw --weights-format=oihw --data-type-cfg=U8F32S8F32 --sampling-kind=2

# a161 Or case modified from resnet50_res2a_branch2b
NSOCKETS=1 ./scripts/run.sh -c -i64 -h56 -o66 -H56 -n1 --tile-size=6 --execution-mode=0xa161 --blk-i=4 --blk-o=1 --flt-o=1 --flt-t=7 --pat-o=1 --output-as-blocked=true $COMMON --input-format=nchw --weights-format=oihw --output-format=nchw --data-type-cfg=U8F32S8F32 --sampling-kind=2

# a133 Ir case modified from resnet50_res5a_branch2b
NSOCKETS=1 ./scripts/run.sh -c -i516 -h7 -o512 -H7 -n1 --tile-size=4 --execution-mode=0xa133 --blk-i=33 --blk-o=1 --flt-o=2 --flt-t=8 $COMMON --input-format=nchw --weights-format=oihw --data-type-cfg=U8F32S8F32 --sampling-kind=2

# a133 Or case modified from resnet50_res5a_branch2b
NSOCKETS=1 ./scripts/run.sh -c -i512 -h7 -o516 -H7 -n1 --tile-size=4 --execution-mode=0xa133 --blk-i=32 --blk-o=1 --flt-o=1 --flt-t=8 $COMMON --input-format=nchw --weights-format=oihw --output-format=nchw --data-type-cfg=U8F32S8F32 --sampling-kind=2
