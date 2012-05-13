<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title></title>
</head>
<style type="text/css">
<!--
body {
	background-image: url(../../image/bg1.png);
	background-repeat: repeat-x;
}
div#wrap {
    height:45px;
    text-align:center;
 }
div#title {
    text-align:center;
    margin-bottom:30px;
 }
div#content {
    vertical-align:middle;
    border:1px solid #FF0099;
    background-color:#FFCCFF;
    width:300px;
    margin-bottom:10px;
 }
-->
</style>
<body>
    <div id="title"><img src="../../image/ba2.png" width="704" height="69" border="0" /></div>
    <table align = center><tr><td>
    <div id="content">球员登记编号：　<%=ViewData["Pid"] %></div>
    <div id="content">姓名：　　　　　<%=ViewData["PlayerName"] %></div>
    <div id="content">中文姓名：　　　<%=ViewData["ChineseName"]%></div>
    <div id="content">身高：　　　　　<%=ViewData["Hight"]%></div>
    <div id="content">体重：　　　　　<%=ViewData["Weight"]%></div>
    <div id="content">大学：　　　　　<%=ViewData["University"]%></div>
    <div id="wrap"><%=Html.ActionLink("修改", "Alter", "Player", new {PlayerID = ViewData["Pid"] }, null)%></div>
    </td></tr></table>
</body>
</html>
