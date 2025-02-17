import numpy as np
from nias.base.operators import IdentityOperator, OperatorBasedInnerProduct
from nias.bindings.numpy.vectorarrays import NumpyVectorSpace
from nias.linalg.gram_schmidt import gram_schmidt

from nias_cpp import double_gram_schmidt_cpp

# create test array
test_array_1 = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]], dtype=np.float64)
test_array_2 = test_array_1.copy()

# apply C++ gram_schmidt
ortho_test_array_1 = double_gram_schmidt_cpp(test_array_1)

# apply Python gram_schmidt
numpy_space = NumpyVectorSpace(test_array_2.shape[0])
ortho_test_array_2 = gram_schmidt(
    numpy_space.from_numpy(test_array_2), inner_product=OperatorBasedInnerProduct(IdentityOperator(numpy_space))
)

# compare results
if np.isclose(ortho_test_array_1, ortho_test_array_2.impl.to_numpy(ensure_copy=False, ind=None)).all():
    print("Results are equal")
else:
    raise ValueError("Results are not equal")
