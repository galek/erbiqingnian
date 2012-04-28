<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title></title>
</head>
<body>
    <div>
    <%foreach (KarottoXros.Models.NewsModels news in ViewData["NewsList"] as List<KarottoXros.Models.NewsModels>)
      { %>
      <p><%=Html.ActionLink(news.Topic, "Show", "News", new {NewsId = news.Id},null)%></p>
    <%} %>
    </div>
</body>
</html>
