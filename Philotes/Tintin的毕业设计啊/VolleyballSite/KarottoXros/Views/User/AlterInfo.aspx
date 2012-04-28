<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title></title>
</head>
<body>
     <div>
    <%using (Html.BeginForm("AlterInfoAction", "User"))
      { %>
    <div>用户名:<%=Html.TextBox("TextName",ViewData["UserName"])%></div>
    <div>原密码:<%=Html.Password("TextPwd")%></div>
    <div>新密码：<%=Html.Password("TextPwd2")%></div>
    <div>确认密码：<%=Html.Password("TextPwd3")%></div>
    <div><input type="submit" value="修改">&nbsp;<input type="reset" value="重置"></div>
    <%} %>
    </div>
</body>
</html>
