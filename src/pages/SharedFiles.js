import React, { useState, useEffect } from 'react';
import { Row, Col, Button, message } from 'antd';
import { SaveOutlined, DownloadOutlined, ShareAltOutlined } from '@ant-design/icons';
import styled from '@emotion/styled';

const ImageContainer = styled.div`
  position: relative;
  width: 100%;
  aspect-ratio: 1;
  overflow: hidden;
  border-radius: 8px;
  cursor: pointer;
  background: rgba(255, 255, 255, 0.75);  // 改为白色背景
  backdrop-filter: blur(5px);
  border: 1px solid rgba(255, 255, 255, 0.3);
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
  transition: transform 0.3s ease;

  &:hover {
    transform: translateY(-5px);
    
    .overlay {
      opacity: 1;
    }
  }

  img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }
`;

const Overlay = styled.div`
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(255, 255, 255, 0.2);  // 改为更透明的无色背景
  backdrop-filter: blur(5px);  // 增加模糊效果
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  opacity: 0;
  transition: opacity 0.3s ease;
  padding: 16px;
  color: #1a5d1a;

  .info {
    text-align: center;
    margin-bottom: 16px;
    background: rgba(255, 255, 255, 0.8);  // 为文字添加半透明背景
    padding: 12px;
    border-radius: 8px;
  }

  .actions {
    display: flex;
    gap: 8px;

    .ant-btn {
      background: rgba(255, 255, 255, 0.8);  // 调整按钮背景为半透明
      border: 1px solid rgba(46, 125, 50, 0.3);
      color: #1a5d1a;

      &:hover {
        background: rgba(255, 255, 255, 0.9);
        border-color: rgba(46, 125, 50, 0.5);
        color: #2e7d32;
      }
    }
  }
`;

const SharedFiles = () => {
  const [files, setFiles] = useState([]);

  useEffect(() => {
    // 模拟获取数据
    setFiles([
      {
        id: '1',
        url: 'https://picsum.photos/400/400',
        name: 'image1.jpg',
        owner: 'User1',
        downloads: 10,
        size: '2.5MB'
      },
      {
        id: '2',
        url: 'https://picsum.photos/401/401',
        name: 'image2.jpg',
        owner: 'User2',
        downloads: 15,
        size: '1.8MB'
      }
    ]);
  }, []);

  const handleDownload = (file) => {
    const link = document.createElement('a');
    link.href = file.url;
    link.download = file.name;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    message.success('开始下载');
  };

  const handleSave = (fileId) => {
    message.success('已转存到我的文件');
  };

  const handleShare = (fileId) => {
    const shareLink = `${window.location.origin}/share/${fileId}`;
    navigator.clipboard.writeText(shareLink);
    message.success('分享链接已复制');
  };

  return (
    <Row gutter={[8, 8]}>  {/* 将间距从默认的 16 改为 8 */}
      {files.map(file => (
        <Col xs={12} sm={8} md={6} lg={4} key={file.id}>  {/* 调整列宽，使图片更紧凑 */}
          <ImageContainer>
            <img src={file.url} alt={file.name} />
            <Overlay className="overlay">
              <div className="info">
                <div className="title">{file.name}</div>
                <div className="details">
                  分享者：{file.owner}<br />
                  大小：{file.size}<br />
                  下载：{file.downloads}次
                </div>
              </div>
              <div className="actions">
                <Button type="primary" ghost icon={<SaveOutlined />} onClick={() => handleSave(file.id)}>
                  转存
                </Button>
                <Button type="primary" ghost icon={<DownloadOutlined />} onClick={() => handleDownload(file)}>
                  下载
                </Button>
                <Button type="primary" ghost icon={<ShareAltOutlined />} onClick={() => handleShare(file.id)}>
                  分享
                </Button>
              </div>
            </Overlay>
          </ImageContainer>
        </Col>
      ))}
    </Row>
  );
};

export default SharedFiles;