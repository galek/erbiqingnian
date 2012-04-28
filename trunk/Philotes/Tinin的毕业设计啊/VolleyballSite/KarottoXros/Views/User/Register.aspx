<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title></title>
</head>
<body>
    <div>
    <%using (Html.BeginForm("RegisterAction", "User"))
      { %>
    <div>用户名:<%=Html.TextBox("TextName")%></div>
    <div>密码： <%=Html.Password("TextPwd")%></div>
    <div>确认密码：<%=Html.Password("TextPwd2")%></div>
    <div><input type="submit" value="提交">&nbsp;<input type="reset" value="重置"></div>
    <%} %>
    </div>
</body>
</html>
