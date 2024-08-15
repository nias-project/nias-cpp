import numpy as np
from nias_cpp import double_gram_schmidt_cpp

test_array = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]], dtype=np.float64)
double_gram_schmidt_cpp(test_array)
print(test_array)
