#include <ATen/ATen.h>
#include <ATen/Config.h>
#include <ATen/NativeFunctions.h>
#include <tuple>

#if !AT_MKLDNN_ENABLED()

namespace at {
namespace native {

std::tuple<Tensor, Tensor> mkldnn_max_pool2d_with_indices(
    const Tensor& self,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    IntArrayRef dilation,
    bool ceil_mode) {
  AT_ERROR(
      "mkldnn_max_pool2d_with_indices: ATen not compiled with MKLDNN support");
}

std::tuple<Tensor&, Tensor&> mkldnn_max_pool2d_with_indices_out(
    Tensor& output,
    Tensor& indices,
    const Tensor& self,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    IntArrayRef dilation,
    bool ceil_mode) {
  AT_ERROR(
      "mkldnn_max_pool2d_with_indices_out: ATen not compiled with MKLDNN support");
}

Tensor mkldnn_avg_pool2d(
    const Tensor& self,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    bool ceil_mode,
    bool count_include_pad) {
  AT_ERROR("mkldnn_avg_pool2d: ATen not compiled with MKLDNN support");
}

Tensor& mkldnn_avg_pool2d_out(
    Tensor& output,
    const Tensor& self,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    bool ceil_mode,
    bool count_include_pad) {
  AT_ERROR("mkldnn_avg_pool2d_out: ATen not compiled with MKLDNN support");
}
} // namespace native
} // namespace at

#else // AT_MKLDNN_EBABLED

#include <ATen/native/mkldnn/MKLDNNCommon.h>
#include <ATen/native/mkldnn/Utils.h>

namespace at {
namespace native {

static Tensor ideep_pool2d(
    const Tensor& input,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    IntArrayRef dilation,
    bool ceil_mode,
    ideep::algorithm algo) {
  auto kernel_size_vec = expand_param_if_needed(kernel_size, 2);
  auto stride_vec = expand_param_if_needed(stride, 2);
  auto padding_vec = expand_param_if_needed(padding, 2);
  auto dilation_vec = expand_param_if_needed(dilation, 2);

  const ideep::tensor& x = itensor_from_mkldnn(input);
  const std::vector<int64_t> output_sizes = pool_output_sizes(
      input.sizes(),
      kernel_size_vec,
      stride_vec,
      padding_vec,
      dilation_vec,
      ceil_mode);
  ideep::tensor y;
  ideep::pooling_forward::compute<AllocForMKLDNN>(
      x,
      {output_sizes.cbegin(), output_sizes.cend()},
      y,
      {stride_vec.cbegin(), stride_vec.cend()},
      {kernel_size_vec.cbegin(), kernel_size_vec.cend()},
      {padding_vec.cbegin(), padding_vec.cend()},
      {padding_vec.cbegin(), padding_vec.cend()},
      algo,
      ideep::prop_kind::forward);

  return new_with_itensor_mkldnn(std::move(y), input.options());
}

std::tuple<Tensor, Tensor> mkldnn_max_pool2d_with_indices(
    const Tensor& input,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    IntArrayRef dilation,
    bool ceil_mode) {
  Tensor output = ideep_pool2d(
      input,
      kernel_size,
      stride,
      padding,
      dilation,
      ceil_mode,
      ideep::algorithm::pooling_max);
  return std::make_tuple(
      output,
      // TODO: ideep currently doesn't expose the max indices,
      // so return garbage values here
      at::empty({0}, at::kInt));
}

std::tuple<Tensor&, Tensor&> mkldnn_max_pool2d_with_indices_out(
    Tensor& output,
    Tensor& indices,
    const Tensor& input,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    IntArrayRef dilation,
    bool ceil_mode) {
  output = ideep_pool2d(
      input,
      kernel_size,
      stride,
      padding,
      dilation,
      ceil_mode,
      ideep::algorithm::pooling_max);
  // TODO: ideep currently doesn't expose the max indices,
  // so return garbage values here
  indices = at::empty({0}, at::kInt);
  return std::tuple<Tensor&, Tensor&>(output, indices);
}

Tensor mkldnn_avg_pool2d(
    const Tensor& input,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    bool ceil_mode,
    bool count_include_pad) {
  return ideep_pool2d(
      input,
      kernel_size,
      stride,
      padding,
      std::vector<int64_t>{1, 1},
      ceil_mode,
      count_include_pad ? ideep::algorithm::pooling_avg_include_padding
                        : ideep::algorithm::pooling_avg_exclude_padding);
}

Tensor& mkldnn_avg_pool2d_out(
    Tensor& output,
    const Tensor& input,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    bool ceil_mode,
    bool count_include_pad) {
  output = ideep_pool2d(
      input,
      kernel_size,
      stride,
      padding,
      std::vector<int64_t>{1, 1},
      ceil_mode,
      count_include_pad ? ideep::algorithm::pooling_avg_include_padding
                        : ideep::algorithm::pooling_avg_exclude_padding);
  return output;
}

} // namespace native
} // namespace at

#endif // AT_MKLDNN_EBABLED
