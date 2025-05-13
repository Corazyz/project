bm_status_t convert(Mat &m, std::vector<Rect> &vrt, std::vector<Size> &vsz, std::vector<Mat> &out, bool update,
        csc_type_t csc, csc_matrix_t *matrix, bmcv_resize_algorithm algorithm)
{
  if (!m.u || !m.u->addr) {printf("Memory allocated by user, no device memory assigned. Not support BMCV!\n"); return BM_NOT_SUPPORTED;}
  bm_status_t ret = BM_SUCCESS;
  bm_handle_t handle = m.u->hid ? m.u->hid : getCard();

  CV_Assert(vrt.size() == vsz.size());
  CV_Assert(out.size() == 0 || out.size() == vsz.size());

  bm_image src;
  toBMI(m, &src, update);

  if (out.size() == 0) {
    for (unsigned int i = 0; i < vrt.size(); i++) {
      Mat ma(vsz[i].height, vsz[i].width, CV_8UC3, SophonDevice(getId(handle)));
      out.push_back(ma);
    }
  } else {
    for (unsigned int i = 0; i < vrt.size(); i++) {
      int id = getId(handle);
      if (out[i].card) id = BM_MAKEFLAG(0, BM_CARD_HEAP(out[i].card), id);

      if (out[i].empty())
        out[i].create(vsz[i].height, vsz[i].width, CV_8UC3, id);
    }
  }

  bm_image *dst = new bm_image[vrt.size()];
  for (unsigned int i = 0; i < vrt.size(); i++) {
    toBMI(out[i], dst + i, false);
  }

  bmcv_rect_t *brt = new bmcv_rect_t[vrt.size()];

  for (unsigned int i = 0; i < vrt.size(); i++) {
    brt[i].start_x = vrt[i].x;
    brt[i].start_y = vrt[i].y;
    brt[i].crop_w = vrt[i].width;
    brt[i].crop_h = vrt[i].height;
  }

  bool has_csc = true, has_convert = false;
  if ((algorithm != BMCV_INTER_NEAREST) && !(m.avOK() && m.avFormat() == AV_PIX_FMT_YUV444P)){
    if (src.image_format == dst[0].image_format)
      has_csc = false;
    if (!(vrt[0].width == m.cols && vrt[0].height == m.rows && vrt.size() == 1 &&
          vrt[0].width == vsz[0].width && vrt[0].height == vsz[0].height))
      has_convert = true;
  }

  bm_image *dst_csc = NULL;
  int output_num;
  bmcv_resize_algorithm alg;
  bmcv_rect_t *rt;
  if (has_convert && has_csc){
    dst_csc = new bm_image;
    ret = bm_image_create(handle, m.rows, m.cols, dst[0].image_format, DATA_TYPE_EXT_1N_BYTE, dst_csc, NULL);
    if (ret != BM_SUCCESS) goto done;

    output_num = 1;
    alg = BMCV_INTER_NEAREST;
    rt = NULL;
  }else if (!has_convert && !has_csc) {   // bypass
    if (update) {
      download(handle, m, &src);
    }
    out[0] = m;
    goto done;
  }else if(has_csc){
    dst_csc = dst;
    output_num = vrt.size();
    alg = algorithm;
    rt = brt;
  }else
    dst_csc = &src;

#ifdef USING_SOC
  for (unsigned int i = 0; i < vrt.size(); i++) {
    download(handle, out[i], &dst[i]); // clear cache at soc mode
  }
#endif
 // step1 - csc
  if (has_csc) {
    ret = bmcv_image_vpp_csc_matrix_convert(handle, output_num, src, dst_csc, csc, matrix, alg, rt);
    if (ret != BM_SUCCESS) goto done;
  }

  // step2 - convert
  if (has_convert){
    ret = bmcv_image_vpp_convert(handle, vrt.size(), *dst_csc, dst, brt, algorithm);
    if (ret != BM_SUCCESS) goto done;
  }

  bm_thread_sync(handle);

#ifndef USING_SOC
  if (update)
    for (unsigned int i = 0; i < vrt.size(); i++) {
      download(handle, out[i], &dst[i]);
    }
#endif

done:
  bm_image_destroy(&src);
  for (unsigned int i = 0; i < vrt.size(); i++) {
    bm_image_destroy(dst+ i);
  }
  if (has_convert && has_csc){
      bm_image_destroy(dst_csc);
      delete dst_csc;
  }

  if (dst) delete[] dst;
  if (brt) delete[] brt;
  return ret;
}
