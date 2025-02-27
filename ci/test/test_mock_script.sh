#!/bin/bash
set -e
MOCK_TORCH=$PWD/python/oneflow/test/misc/test_mock_simple.py

same_or_exit() {
    if [[ "$(python3 $MOCK_TORCH)" != *"$1"* ]]; then
        exit 1
    fi
}
eval $(python3 -m oneflow.mock_torch) # test call to python module, default argument is enable
same_or_exit "True"

# testing import
python3 -c 'import torch; torch.randn(2,3)'
python3 -c 'import torch.nn; torch.nn.Graph'
python3 -c 'import torch.version; torch.version.__version__'
python3 -c 'from torch import *; randn(2,3)'
python3 -c 'from torch.nn import *; Graph'
python3 -c 'from torch.sbp import *; sbp'
python3 -c 'from torch import nn; nn.Graph'
python3 -c 'from torch.version import __version__'
python3 -c 'import torch; torch.no_exist' 2>&1 >/dev/null | grep -q 'NotImplementedError'

eval $(python3 -m oneflow.mock_torch disable)
same_or_exit "False"
eval $(python3 -m oneflow.mock_torch enable)
same_or_exit "True"
eval $(python3 -m oneflow.mock_torch disable) # recover
same_or_exit "False"
eval $(oneflow-mock-torch) # test scripts
same_or_exit "True"
eval $(oneflow-mock-torch disable)
same_or_exit "False"
eval $(oneflow-mock-torch enable)
same_or_exit "True"
eval $(oneflow-mock-torch disable)
same_or_exit "False"
