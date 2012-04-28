<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title></title>
</head>
<body>
    <div>
    <div>球员登记编号<%=ViewData["Pid"] %></div>
    <div>姓名<%=ViewData["PlayerName"] %></div>
    <div>中文姓名<%=ViewData["ChineseName"]%></div>
    <div>身高<%=ViewData["Hight"]%></div>
    <div>体重<%=ViewData["Weight"]%></div>
    <div>大学<%=ViewData["University"]%></div>
    <div>主要位置<%=ViewData["MainLocation"]%></div>
    <div>擅长位置<%foreach (string str in ViewData["LocationList"] as List<string>)
           { %>
           <%=str    %>
    <%} %></div>
    <div><%=Html.ActionLink("修改", "Alter", "Player", new {PlayerID = ViewData["Pid"] }, null)%></div>
    </div>
</body>
</html>
