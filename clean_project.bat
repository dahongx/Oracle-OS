@echo off
echo ========================================
echo Oracle OS 项目清理脚本（保守方案）
echo ========================================
echo.

echo [1/5] 删除测试图片目录 (test/)...
rmdir /s /q test 2>nul

echo [2/5] 删除临时字体目录 (tmp/)...
rmdir /s /q tmp 2>nul

echo [3/5] 删除供应商示例代码...
rmdir /s /q "思迈微1.54寸-12P插接技术手册" 2>nul
rmdir /s /q "hardware\doc\屏幕\_extracted" 2>nul

echo [4/5] 删除 Claude 会话日志...
del /q *.txt 2>nul

echo [5/5] 清理编译产物...
rmdir /s /q "firmware\esp32s3_knob_idf\build" 2>nul

echo.
echo ========================================
echo 清理完成！
echo ========================================
echo.
echo 已删除：
echo   - test/ (测试图片)
echo   - tmp/ (临时字体)
echo   - 供应商示例代码
echo   - Claude 会话日志
echo   - 编译产物
echo.
echo 保留：
echo   - archive/ (历史记录)
echo   - oracle_os_web_checked(1)/ (网页演示)
echo   - .gitignore 已创建
echo.
pause
