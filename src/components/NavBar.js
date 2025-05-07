import React from 'react';
import { Layout, Menu } from 'antd';
import { Link, useLocation } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';
import styled from '@emotion/styled';
import { 
  HomeOutlined, 
  PictureOutlined, 
  ShareAltOutlined, 
  DownloadOutlined,
  UserOutlined
} from '@ant-design/icons';

const { Header } = Layout;

const StyledHeader = styled(Header)`
  background: rgba(144, 238, 144, 0.25);
  backdrop-filter: blur(10px);
  border-bottom: 1px solid rgba(255, 255, 255, 0.5);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  display: flex;
  align-items: center;
`;

const StyledMenu = styled(Menu)`
  background: transparent !important;

  .ant-menu-item {
    color: #1a5d1a !important;
    
    &:hover {
      color: #2e7d32 !important;
    }
    
    .anticon {
      color: #1a5d1a !important;
    }
    
    a {
      color: #1a5d1a !important;
      &:hover {
        color: #2e7d32 !important;
      }
    }
  }
`;

const Logo = styled.div`
  color: #1a5d1a;
  font-weight: bold;
  font-size: 18px;
  margin-right: 24px;
`;

const MainMenu = styled(StyledMenu)`
  flex: 1;
`;

const UserMenu = styled(StyledMenu)`
  min-width: 120px;
  margin-left: auto;
  
  .ant-menu-item, .ant-menu-submenu-title {
    display: flex !important;
    align-items: center !important;
    justify-content: center !important;
    padding: 0 16px !important;
    
    .anticon {
      margin-right: 8px;
      font-size: 16px;
    }

    a {
      display: flex !important;
      align-items: center !important;
      gap: 8px;
      font-size: 14px;
      font-weight: 500;
    }
  }

  &.ant-menu-horizontal {
    border-bottom: none;
    line-height: 46px;
  }
`;


const NavBar = () => {
  const { user, logout } = useAuth();
  const location = useLocation();

  const menuItems = [
    { key: '/', icon: <HomeOutlined />, label: '首页' },
    { key: '/images', icon: <PictureOutlined />, label: '我的图片' },
    { key: '/shared', icon: <ShareAltOutlined />, label: '共享文件' },
    { key: '/top-downloads', icon: <DownloadOutlined />, label: '下载榜' },
  ];

  return (
    <StyledHeader>
      <Logo>图床系统</Logo>
      <MainMenu
        mode="horizontal"
        selectedKeys={[location.pathname]}
        items={menuItems.map(item => ({
          ...item,
          label: <Link to={item.key}>{item.label}</Link>
        }))}
      />
      {user ? (
        <UserMenu
          mode="horizontal"
          items={[
            {
              key: 'user',
              icon: <UserOutlined />,
              label: user.username,
              children: [
                {
                  key: 'logout',
                  label: '退出登录',
                  onClick: logout
                }
              ]
            }
          ]}
        />
      ) : (
        <UserMenu
          mode="horizontal"
          items={[
            {
              key: 'login',
              label: (
                <Link to="/login">
                  <UserOutlined />
                  <span>登录</span>
                </Link>
              )
            }
          ]}
        />
      )}
    </StyledHeader>
  );
};

export default NavBar;