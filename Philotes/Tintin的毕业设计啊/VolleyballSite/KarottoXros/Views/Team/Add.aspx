<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title></title>
</head>
<body>
    <div>
    <%using (Html.BeginForm("AddAction", "Team"))
      { %>
    <div>球队名称<%=Html.TextBox("Name")%></div>
    <div>所在城市<%=Html.TextBox("City")%></div>
    <div>所在地区<%=Html.DropDownList("location")%></div>
    <div>简介<%=Html.TextArea("Info")%></div>
    <div><input type="submit" value="提交" /></div>
    <%} %>
    <div></div>
    </div>
</body>
</html>
