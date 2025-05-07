import React, { useState, useEffect } from 'react';
import { Upload, Card, Row, Col, Modal, message, Tooltip } from 'antd';
import { PlusOutlined, ShareAltOutlined, DeleteOutlined, DownloadOutlined } from '@ant-design/icons';
import styled from '@emotion/styled';
import { useAuth } from '../contexts/AuthContext';  // 添加这行导入
import SparkMD5 from 'spark-md5';
import { fetchUserImages, uploadImage, deleteImage } from '../services/images';

const ImageGrid = styled(Row)`
  margin-top: 20px;
`;

const UploadCard = styled(Card)`
  width: 100%;
  height: 100%;
  .ant-upload-select {
    width: 100%;
    height: 200px;
  }
`;

const ImageCard = styled(Card)`
  margin-bottom: 16px;
  .ant-card-cover {
    height: 200px;
    overflow: hidden;
    display: flex;
    align-items: center;
    justify-content: center;
    img {
      max-width: 100%;
      max-height: 100%;
      object-fit: contain;
    }
  }
`;



const ImageList = () => {
  const [images, setImages] = useState([]);
  const [previewVisible, setPreviewVisible] = useState(false);
  const [previewImage, setPreviewImage] = useState('');
  const [previewTitle, setPreviewTitle] = useState('');
  const { user } = useAuth();  // 获取用户信息

  const fetchImages = async () => {
    try {
      const filesWithUrls = await fetchUserImages(user);
      setImages(filesWithUrls);
    } catch (error) {
      console.error('获取图片列表错误：', error);
      message.error('获取图片列表失败，请检查网络连接');
    }
  };

  const handleUpload = async (file) => {
    try {
      await uploadImage(file, user);
      message.success('上传成功！');
      fetchImages();
    } catch (error) {
      console.error('上传错误：', error);
      message.error('上传失败！');
    }
  };

  const handleDelete = async (image) => {
    try {
      await deleteImage(image, user);
      message.success('删除成功！');
      fetchImages();
    } catch (error) {
      console.error('删除错误：', error);
      message.error('删除失败！');
    }
  };

  useEffect(() => {
    if (user && user.token) {
      fetchImages();
    }
  }, [user]);

  const handlePreview = (file) => {
    setPreviewImage(file.url);
    setPreviewTitle(file.name);
    setPreviewVisible(true);
  };

  const handleShare = async (imageId) => {
    try {
      // 实现分享逻辑(其实就是图床的地址+文件名)
      message.success('分享链接已复制到剪贴板！');
    } catch (error) {
      message.error('分享失败！');
    }
  };

  // const handleDelete = async (image) => {
  //   try {
  //     console.log("image", image);
  //     const response = await fetch('http://192.168.88.124:8080/api/dealfile?cmd=del', {
  //       method: 'POST',
  //       headers: {
  //         'Content-Type': 'application/json'
  //       },
  //       body: JSON.stringify({
  //         token: user.token,
  //         user: user.username,
  //         md5: image.md5,
  //         filename: image.file_name
  //       })
  //     });
  
  //     const data = await response.json();
  //     if (data.code === 0) {
  //       message.success('删除成功！');
  //       fetchImages();  // 刷新列表
  //     } else {
  //       message.error(data.msg || '删除失败！');
  //     }
  //   } catch (error) {
  //     console.error('删除错误：', error);
  //     message.error('删除失败！');
  //   }
  // };

  const handleDownload = async (image) => {
    try {
      const link = document.createElement('a');
      link.href = image.url;
      link.download = image.name;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    } catch (error) {
      message.error('下载失败！');
    }
  };

  return (
    <div>
      <Row gutter={[16, 16]}>
        <Col xs={24} sm={12} md={8} lg={6}>
          <UploadCard>
            <Upload.Dragger
              accept="image/*"
              showUploadList={false}
              beforeUpload={(file) => {
                handleUpload(file);
                return false;
              }}
            >
              <p><PlusOutlined /></p>
              <p>点击或拖拽上传图片</p>
            </Upload.Dragger>
          </UploadCard>
        </Col>
        {images.map(image => (
          <Col xs={24} sm={12} md={8} lg={6} key={image.id}>
            <ImageCard
              cover={<img alt={image.name} src={image.url} onClick={() => handlePreview(image)} />}
              actions={[
                <Tooltip title="分享">
                  <ShareAltOutlined onClick={() => handleShare(image.id)} />
                </Tooltip>,
                <Tooltip title="下载">
                  <DownloadOutlined onClick={() => handleDownload(image)} />
                </Tooltip>,
                <Tooltip title="删除">
                  <DeleteOutlined onClick={() => handleDelete(image)} />
                </Tooltip>,
              ]}
            >
              <Card.Meta title={image.name} description={`下载次数：${image.pv}`} />
            </ImageCard>
          </Col>
        ))}
      </Row>

      <Modal
        visible={previewVisible}
        title={previewTitle}
        footer={null}
        onCancel={() => setPreviewVisible(false)}
      >
        <img alt={previewTitle} style={{ width: '100%' }} src={previewImage} />
      </Modal>
    </div>
  );
};

export default ImageList;