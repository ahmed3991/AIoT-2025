(Task 5.1)Report and conclusion

| Metric                                     |       MLP |       CNN |
| ------------------------------------------ | --------: | --------: |
| Test Accuracy                              |    0.8667 |    0.8826 |
| Trainable Parameters                       |   235,146 |    56,714 |
| Saved Model Size (MB)                      |     0.919 |     0.248 |
| FLOPs (Training) / image                   | 1,879,592 | 5,713,064 |
| FLOPs (Inference) / image                  |   469,898 | 1,428,266 |
| Training Memory (MB) *(Params+Grads+Adam)* |     3.588 |     0.865 |
                               **0.865** |

Answers

Which model achieved higher accuracy?
CNN with an accuracy of 0.8826 vs 0.8667 for MLP.

Which model had fewer parameters (lower storage footprint)?
CNN: ~56.7K parameters and 0.248 MB file size, vs 235K and 0.919 MB for MLP.

Analysis and trade-off, and why CNNs are usually better for images

Why did the CNN win on accuracy?
Because CNNs exploit the spatial structure of the image:

Weight sharing across filters reduces parameters and improves generalization.

Locality: filters learn edges/textures/local shapes that form strong class features.

Convolutional equivariance provides robustness to small translations/changes.

MLP’s advantage in this setup:

Cheaper per-image compute (MLP inference FLOPs ≈ 0.47M < CNN ≈ 1.43M) and simple to implement.

But it flattens the image and loses spatial information, making it less expressive for visual patterns and often requiring dense connections that increase parameters.

Conclusion:
For image tasks, CNNs typically achieve higher accuracy and fewer parameters thanks to convolution and pooling, even if they require more inference FLOPs than a simple MLP.