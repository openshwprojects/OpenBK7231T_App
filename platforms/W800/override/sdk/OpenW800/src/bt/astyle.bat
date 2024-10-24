REM ��������Ŀ¼�е�����C++�ļ���Astyle���д�����������
REM ����Astyle����λ�úͲ���
@echo off
set astyle="astyle.exe"
REM ѭ������Ŀ¼
REM for /r . %%a in (*.cpp;*.c) do %astyle% --style=bsd --convert-tabs --indent=spaces=4 --attach-closing-while --indent-switches --indent-namespaces --indent-continuation=4 --indent-preproc-block  --indent-col1-comments --pad-oper  --unpad-paren --delete-empty-lines --align-pointer=name --align-reference=name  --add-braces --pad-comma --add-one-line-braces -s4 -n "%%a"
REM for /r . %%a in (*.hpp;*.h) do %astyle% --style=bsd --convert-tabs --indent=spaces=4 --attach-closing-while --indent-switches --indent-namespaces --indent-continuation=4 --indent-preproc-block  --indent-col1-comments --pad-oper  --unpad-paren --delete-empty-lines --align-pointer=name --align-reference=name  --add-braces --pad-comma --add-one-line-braces -s4 -n "%%a"
for /r . %%a in (*.cpp;*.c) do %astyle% -A10 --max-code-length=100 --convert-tabs --indent=spaces=4 --attach-closing-while --indent-switches --indent-namespaces --indent-continuation=4  --indent-col1-comments --pad-oper  --unpad-paren --delete-empty-lines --align-pointer=name --align-reference=name  --add-braces --pad-comma --add-one-line-braces -s4 -n --break-blocks "%%a"
for /r . %%a in (*.hpp;*.h) do %astyle% -A10 --max-code-length=100 --convert-tabs --indent=spaces=4 --attach-closing-while --indent-switches --indent-namespaces --indent-continuation=4  --indent-col1-comments --pad-oper  --unpad-paren --delete-empty-lines --align-pointer=name --align-reference=name  --add-braces --pad-comma --add-one-line-braces -s4 -n --break-blocks "%%a"

REM ɾ�����е�astyle�����ļ�
for /r . %%a in (*.orig) do del "%%a"
pause