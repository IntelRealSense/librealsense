ARG FROM_IMAGE=ros:dashing

# multi-stage for caching
FROM $FROM_IMAGE AS cache

# copy overlay source
ENV OVERLAY_WS /opt/overlay_ws
RUN mkdir -p $OVERLAY_WS/src
WORKDIR $OVERLAY_WS
COPY ./ src/librealsense

# copy manifests for caching
WORKDIR /opt
RUN find ./ -name "package.xml" | \
      xargs cp --parents -t /tmp
    # && find ./ -name "COLCON_IGNORE" | \
    #   xargs cp --parents -t /tmp

# multi-stage for building
FROM $FROM_IMAGE AS build

# install CI dependencies
RUN apt-get update && apt-get install -q -y \
      ccache \
      lcov \
    && rm -rf /var/lib/apt/lists/*

# copy overlay manifests
ENV OVERLAY_WS /opt/overlay_ws
COPY --from=cache /tmp/overlay_ws $OVERLAY_WS
WORKDIR $OVERLAY_WS

# install overlay dependencies
RUN . /opt/ros/$ROS_DISTRO/setup.sh && \
    apt-get update && rosdep install -q -y \
      --from-paths src \
      --ignore-src \
    && rm -rf /var/lib/apt/lists/*

# copy overlay source
COPY --from=cache $OVERLAY_WS ./

# build overlay source
ARG OVERLAY_MIXINS="release ccache"
RUN . /opt/ros/$ROS_DISTRO/setup.sh && \
    colcon build \
      --symlink-install \
      --mixin \
        $OVERLAY_MIXINS \
      --event-handlers console_direct+

# source overlay from entrypoint
RUN sed --in-place \
      's|^source .*|source "$OVERLAY_WS/install/setup.bash"|' \
      /ros_entrypoint.sh