@echo off
echo ========================================
echo Oracle OS 项目清理脚本（彻底方案）
echo ========================================
echo.

echo [1/7] 删除测试图片目录 (test/)...
rmdir /s /q test 2>nul

echo [2/7] 删除临时字体目录 (tmp/)...
rmdir /s /q tmp 2>nul

echo [3/7] 删除供应商示例代码...
rmdir /s /q "思迈微1.54寸-12P插接技术手册" 2>nul
rmdir /s /q "hardware\doc\屏幕\_extracted" 2>nul

echo [4/7] 删除独立网页项目...
rmdir /s /q "oracle_os_web_checked(1)" 2>nul

echo [5/7] 删除历史对话记录...
rmdir /s /q archive 2>nul

echo [6/7] 删除 Claude 会话日志...
del /q *.txt 2>nul

echo [7/7] 清理编译产物...
rmdir /s /q "firmware\esp32s3_knob_idf\build" 2>nul

echo.
echo ========================================
echo 彻底清理完成！节省约 100MB 空间
echo ========================================
echo.
echo 已删除所有临时文件和编译产物
echo .gitignore 已创建，可以初始化 Git 仓库
echo.
pause
