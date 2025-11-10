# TP5 Report: Fashion-MNIST Summary

I ran `tp5_solution.py` for five epochs on each model. The table below lists the numbers I observed on my machine.

| Model | Test Accuracy | Trainable Parameters | Saved Model Size (MB) | FLOPs (Training) | FLOPs (Inference) | Training Memory (MB) |
| :---: | :-----------: | :------------------: | :-------------------: | :--------------: | :---------------: | :-------------------: |
| MLP | 0.8790 | 235146 | 0.90 | 9.39e+05 | 4.70e+05 | 3.59 |
| CNN | 0.8940 | 56714 | 0.22 | 2.83e+06 | 1.41e+06 | 0.87 |

## Quick notes

1. The CNN finished with the best accuracy because convolution filters pay attention to local patterns such as edges and textures.
2. Even though the CNN uses more FLOPs per step, the parameter count stays low, so the saved file is smaller than the MLP.
3. The MLP trains a little faster per epoch, but without convolutions it cannot match the spatial awareness of the CNN, which is why CNNs are usually preferred for image tasks.

