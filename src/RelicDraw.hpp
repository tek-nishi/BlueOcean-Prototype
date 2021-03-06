﻿#pragma once

//
// ステージ上の遺物の描画
//

namespace ngs {

class RelicDrawer {
  float range_;
  
  std::vector<ci::Color> color_;
  ci::gl::GlslProgRef shader_;

  ci::gl::VboMeshRef model_;
  ci::gl::VboMeshRef model2_;

  ci::quat rotation_;

  ci::vec3 rotate_speed_;
  

public:
  RelicDrawer(const ci::JsonTree& params)
    : rotate_speed_(Json::getVec<ci::vec3>(params["rotate_speed"]))
  {
    for (size_t i = 0; i < params["color"].getNumChildren(); ++i) {
      color_.push_back(Json::getColor<float>(params["color"][i]));
    }
    
    // 計算量を減らすため２乗した値を保存
    float range = params.getValueForKey<float>("range");
    range_ = range * range;
    
    shader_ = createShader("color", "color");

    {
      ci::ObjLoader loader(Asset::load("relic.obj"));
      model_ = ci::gl::VboMesh::create(loader);
    }

    {
      ci::ObjLoader loader(Asset::load("relic_get.obj"));
      model2_ = ci::gl::VboMesh::create(loader);
    }
  }

  
  void setupLight(const Light& light) {
    shader_->uniform("LightPosition", light.direction);
    shader_->uniform("LightAmbient",  light.ambient);
    shader_->uniform("LightDiffuse",  light.diffuse);
  }

  
  void update() {
    rotation_ = rotation_ * ci::quat(rotate_speed_);
  }
  
  void draw(const std::vector<Relic>& relics, const ci::vec3& offset, const ci::vec3& center, const float sea_level) {
    if (relics.empty()) return;

    ci::gl::ScopedGlslProg shader(shader_);

    for (const auto& relic : relics) {
      // 船からの距離によるクリッピング
      float dx = relic.position.x - center.x;
      float dz = relic.position.z - center.z;
      if ((dx * dx + dz * dz) > range_) continue;
      
      ci::vec3 pos(relic.position.x, std::max(float(relic.position.y), sea_level), relic.position.z);
      
      // TIPS:マス目の中央に位置するようオフセットを加えている
      ci::mat4 transform = glm::translate(pos + ci::vec3(0.5, 0.5, 0.5) + offset)
        * glm::mat4_cast(rotation_);
      ci::gl::setModelMatrix(transform);

      // 探索すると色が変わる
      float t = std::min(relic.searched_time / relic.search_required_time, 1.0);
      auto color = color_[0].lerp(t, color_[1]);
      ci::gl::color(color);

      ci::gl::draw(relic.searched ? model2_
                                  : model_);
    }
  }

};

}
