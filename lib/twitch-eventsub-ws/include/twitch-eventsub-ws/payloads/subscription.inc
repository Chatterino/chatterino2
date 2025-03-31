boost::json::result_for<Transport, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Transport>, const boost::json::value &jvRoot);

boost::json::result_for<Subscription, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Subscription>,
    const boost::json::value &jvRoot);
