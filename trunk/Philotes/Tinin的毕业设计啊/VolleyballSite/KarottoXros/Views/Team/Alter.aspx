<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title></title>
</head>
<body>
    <div>
    <%using (Html.BeginForm("AlterAction", "Team"))
      { %>
      <div>球队编号<%=Html.TextBox("Tid", ViewData["Tid"], new { readOnly=true})%></div>
      <div>所在城市<%=Html.TextBox("City") %></div>
      <div>球队名<%=Html.TextBox("Name") %></div>
      <div>地区<%=Html.DropDownList("Location")%></div>
      <div>简介<%=Html.TextArea("Info") %></div>
      <input type="submit" value="修改" />
    <%} %>
    </div>
</body>
</html>
